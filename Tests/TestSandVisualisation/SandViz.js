class SandViz {

    constructor(sandSim) {
        this.sandSim = sandSim;
    }
    
    // Redraw sand after movement
    redraw() {
        // Iterate through the dirty locations
        let dirtyPointList = this.sandSim.getDirtyPointList();
        for (let i = 0; i < dirtyPointList.length; i++) {
            console.log("Dirty");
        }
    }

    // Graph
    showContour(x1, y1, x2, y2) {
        let diffX = x2 - x1;
        let diffY = y2 - y1;
        let pts = 100;
        let lineData = [];
        for (let i = 0; i < pts; i++)
        {
            let ptX = x1 + diffX * i / pts;
            let ptY = y1 + diffY * i / pts;
            let val = this.sandSim.sandLevel[Math.floor(ptX)][Math.floor(ptY)];
            lineData.push({x: i, y: val})
        }

        var ctx = document.getElementById("chart-canvas").getContext('2d');
        var scatterChart = new Chart(ctx, {
            type: 'scatter',
            data: {
                datasets: [{
                    label: 'Scatter Dataset',
                    data: lineData
                }]
            },
            options: {
                scales: {
                    xAxes: [{
                        type: 'linear',
                        position: 'bottom'
                    }]
                }
            }
        });
    }
}

function bodyLoaded()
{
    let sandTableSideLen = 500
    let ballDiameter = 20
    let sandSim = new SandSim(ballDiameter, sandTableSideLen);
    let sandViz = new SandViz(sandSim);

    sandSim.placeBallInitially();

    sandViz.showContour(300, 280, 300, 320);
}
