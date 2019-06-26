class SandSim {
    constructor(ballDiameter, sandTableSideLen) {
        this.sandTableSideLen = Math.ceil(sandTableSideLen);
        this.kernelSize = 2 * ballDiameter;
        this.sandLevel = [];
        this.ballPos = [sandTableSideLen/2, sandTableSideLen/2];
        this.ballDiameter = ballDiameter;
        this.sandStartLevel = ballDiameter/4;
        this.maxSandLevel = ballDiameter/2;
        this.dirtyPointList = [];

        // Init
        this.initSand();
    }

    initSand()
    {
        this.createSand();
        this.createKernel();
    }

    createSand()
    {
        // Generate a 2D array of sand
        this.sandLevel = new Array(this.sandTableSideLen);
        for (let i = 0; i < this.sandTableSideLen; i++)
        {
            this.sandLevel[i] = new Array(this.sandTableSideLen);
            for (let j = 0; j < this.sandTableSideLen; j++)
                this.sandLevel[i][j] = this.sandStartLevel;
        }
    }

    createKernel()
    {

    }

    moveSand(x, y, dirn)
    {

    }

    placeBallInitially()
    {
        // Start by moving sand all around the ball (in both forward and reverse direction)
        this.moveSand(this.ballPos[0], this.ballPos[1], 0);
        this.moveSand(this.ballPos[0]+1, this.ballPos[1], Math.PI);

        // Add the ball position to the dirty list
        this.dirtyPointList.push(this.ballPos);
    }

    getDirtyPointList() {
        return this.dirtyPointList;
    }
}
