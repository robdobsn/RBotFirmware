"use strict";
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
}
class SandKernel {
    constructor(numRings, ringSpacing, ballDiameter) {
        this.displacementKernel = [];
        // Create bin-rings that are a list of points in a square grid
        // that are at various distances from the centre of the grid
        let binRings = this.createBinRings(numRings, ringSpacing);
        // Create displacement rings
        this.displacementKernel =
            this.createDisplacementKernel(binRings, ringSpacing, ballDiameter);
    }
    createBinRings(numRings, ringSpacing) {
        // Iterate over a 2*numRings sized square grid
        // For each point in the grid calculate distance from centre
        // Bin these distances into "rings"
        // Make a list of the coordinates of each point in each ring
        let binRings = [];
        for (let i = 0; i < numRings; i++)
            binRings.push([]);
        const gridCentre = { x: 0, y: 0 };
        for (let x = -numRings; x <= numRings; x++) {
            for (let y = -numRings; y <= numRings; y++) {
                let thisPt = { x: x, y: y };
                let ptDist = Math.floor(this.distFrom(thisPt, gridCentre));
                // console.log("Pt", x, y, ptDist);
                if (ptDist < numRings * ringSpacing)
                    binRings[Math.floor(ptDist / ringSpacing)].push(thisPt);
            }
        }
        // for (let i = 0; i < binRings.length; i++)
        //     for (let j = 0; j < binRings[i].length; j++)
        //         console.log("Ring", i, binRings[i][j])
        return binRings;
    }
    displacementLevel(ringIdx, ringSpacing, ballDiameter) {
        let ringDist = ringIdx * ringSpacing;
        let level = 0;
        if (ringDist < ballDiameter / 2) {
            // Ball level based on ball curvature away from centre
            level = Math.sqrt(Math.pow(ballDiameter / 2, 2) - Math.pow(ballDiameter / 2 - ringDist, 2));
        }
        else {
            // Displacement is negative and based on relative height of sand in outer levels
            level = -1;
        }
        return level;
    }
    r2d(rads) {
        return rads * 180 / Math.PI;
    }
    calcAngleBetweenPointVecsDegs(pt1, pt2) {
        // Calculate angle subtended at origin (0,0) between two points
        let a1 = Math.atan2(pt1.x, pt1.y);
        let a2 = Math.atan2(pt2.x, pt2.y);
        if (a1 > a2)
            return this.r2d(a1 - a2);
        return this.r2d(a2 - a1);
    }
    getSandDests(binRings, srcRingIdx, srcPoint, destAngleRangeDegs) {
        // To return
        let destPtAndWts = [];
        // Accumulate angle weights
        let angleWeightAccum = 0;
        // Iterate all possible points in the next available ring outwards from the source point's ring
        for (let destPtIdx = 0; destPtIdx < binRings[srcRingIdx + 1].length; destPtIdx++) {
            // Check if point is within range
            let destPt = binRings[srcRingIdx + 1][destPtIdx];
            let vecAngleDegs = this.calcAngleBetweenPointVecsDegs(srcPoint, destPt);
            if (vecAngleDegs < destAngleRangeDegs / 2) {
                // Calculate angle weight
                let angleWeight = destAngleRangeDegs / 2 - vecAngleDegs;
                angleWeightAccum += angleWeight;
                // Initially assign angleWeight to weight
                destPtAndWts.push({ weight: angleWeight, destPt: destPt });
            }
        }
        // Go over list again calculating sand weights as fractions of angleWeight/total
        for (let sandDestIdx = 0; sandDestIdx < destPtAndWts.length; sandDestIdx++) {
            // Calculate fractional sand movement
            destPtAndWts[sandDestIdx].weight = destPtAndWts[sandDestIdx].weight / angleWeightAccum;
        }
        return destPtAndWts;
    }
    createDisplacementKernel(binRings, ringSpacing, ballDiameter) {
        // Generate a list of sand displacement rings
        // Each ring is a list of sand displacement objects
        // Each sand displacement object defines a level 
        // over which sand will be displaced (also can be -ve and depend on dest-level)
        // & a list of destination points for the sand to move to along with weights
        // to determine how much of the sand moves to each of the dest points
        let displacementKernel = [];
        for (let ringIdx = 0; ringIdx < binRings.length - 1; ringIdx++) {
            // Calculate displacement amount at each ring
            let dispLevel = this.displacementLevel(ringIdx, ringSpacing, ballDiameter);
            // Iterate the points at each ring
            for (let j = 0; j < binRings[ringIdx].length; j++) {
                // Discover the destination points for sand from this point
                let destAngleRangeDegs = 90;
                // Get list of points and weights at the next ring out from this point
                // that are within the angle range
                let sandDests = this.getSandDests(binRings, ringIdx, binRings[ringIdx][j], destAngleRangeDegs);
                // Dest info
                let sandDisp = {
                    level: dispLevel,
                    sourcePt: binRings[ringIdx][j],
                    dests: sandDests
                };
                displacementKernel.push(sandDisp);
            }
        }
        return displacementKernel;
    }
    distFrom(pt1, pt2) {
        return Math.sqrt(Math.pow(pt2.x - pt1.x, 2) + Math.pow(pt2.y - pt1.y, 2));
    }
    moveSand(sandMatrix, pt, angleDegs) {
        // Iterate source points
        for (let i = 0; i < this.displacementKernel.length; i++) {
            // Displacement
            let sandDisp = this.displacementKernel[i];
            // Check if point is within sand matrix
            let srcX = pt.x + sandDisp.sourcePt.x;
            let srcY = pt.y + sandDisp.sourcePt.y;
            if ((srcY >= 0) && (srcY < sandMatrix.length) && (srcX >= 0) && (srcX < sandMatrix[0].length)) {
                // Check level above activation
                if (sandMatrix[srcY][srcX] > sandDisp.level) {
                    let sandToDisperse = sandMatrix[srcY][srcX] - sandDisp.level;
                    sandMatrix[srcY][srcX] = sandDisp.level;
                    // Disperse sand
                    let sandRemaining = sandToDisperse;
                    for (let j = 0; j < sandDisp.dests.length; j++) {
                        // Check validity of destination
                        let dest = sandDisp.dests[j];
                        let destX = dest.destPt.x + pt.x;
                        let destY = dest.destPt.y + pt.y;
                        if ((destY >= 0) && (destY < sandMatrix.length) && (destX >= 0) && (destX < sandMatrix[0].length)) {
                            let moveAmount = sandToDisperse * dest.weight;
                            sandMatrix[destY][destX] += moveAmount;
                            sandRemaining -= moveAmount;
                        }
                    }
                    // Put any remaining sand back on the original point so we don't leak sand
                    // This should only happen at the edges of the sand Matrix
                    sandMatrix[srcY][srcX] += sandRemaining;
                }
            }
        }
    }
}
class SandSim {
    constructor(ballDiameter, sandTableSideLen) {
        this.sandTableSideLen = Math.ceil(sandTableSideLen);
        this.kernelSize = 2 * ballDiameter;
        this.ringSpacing = 1;
        this.sandMatrix = [];
        this.ballPos = { x: sandTableSideLen / 2, y: sandTableSideLen / 2 };
        this.ballDiameter = ballDiameter;
        this.sandStartLevel = ballDiameter / 4;
        this.maxSandLevel = ballDiameter / 2;
        this.dirtyPointList = [];
        // Create a kernel that will be used to move sand
        this.sandKernel = new SandKernel(this.ballDiameter, this.ringSpacing, this.ballDiameter);
        // this.debugCheckKernel();
        // Init
        this.createSand();
    }
    createSand() {
        // Generate a 2D array of sand
        this.sandMatrix = new Array(this.sandTableSideLen);
        for (let i = 0; i < this.sandTableSideLen; i++) {
            this.sandMatrix[i] = new Array(this.sandTableSideLen);
            for (let j = 0; j < this.sandTableSideLen; j++)
                this.sandMatrix[i][j] = this.sandStartLevel;
        }
    }
    debugCheckKernel() {
        for (let i = 0; i < this.sandKernel.displacementKernel.length; i++) {
            let disp = this.sandKernel.displacementKernel[i];
            console.log("Disp level", disp.level, "Pt", disp.sourcePt, "...");
            for (let j = 0; j < disp.dests.length; j++) {
                let dest = disp.dests[j];
                console.log("Dest", dest.weight, "Pt", dest.destPt);
            }
        }
    }
    moveSand(pt, angle) {
        // Move sand at ball position
        this.sandKernel.moveSand(this.sandMatrix, pt, angle);
    }
    placeBallInitially() {
        // Start by moving sand all around the ball (in both forward and reverse direction)
        this.moveSand(this.ballPos, 0);
        // this.moveSand(new Point(this.ballPos.x+1, this.ballPos.y), Math.PI);
        // Add the ball position to the dirty list
        this.dirtyPointList.push(this.ballPos);
    }
    getDirtyPointList() {
        return this.dirtyPointList;
    }
}
// let sandSim = new SandSim(10, 100);
// export { SandSim };
//# sourceMappingURL=SandSim.js.map