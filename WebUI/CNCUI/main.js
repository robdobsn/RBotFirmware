/* Main RBotFirmware CNC control interface
    by Rob Dobson (c) 2017
    MIT License
*/

function bodyIsLoaded()
{
	console.log("Body loaded");
	// Location for program data
	window.RBotUIVars = {
		uploadFile: null,
		uploadInProgress: false,
		MAX_POST_CHUNK_SIZE: 1000,
		UPLOAD_RETRY_TIME_MS: 1000,
		MAX_UPLOAD_RETRIES: 180,
		axisNames: ["X","Y","Z"]
	};
	// Handle the file send box listener
	var inputFileUpload = document.getElementById("input_file_upload");
	inputFileUpload.addEventListener('change', fileUploadChange, false);
	// Status update timer
	window.RBotUIVars.statusUpdateTimer = setTimeout(statusUpdate, 100);
}

function statusUpdate() {
	callAjax("/status", statusCallback);
}

function statusCallback(resp) {
	var machineStatus = JSON.parse(resp);
	var axisNames = window.RBotUIVars.axisNames;
	if (machineStatus.hasOwnProperty("pos"))
    {
        for (var i = 0; i < axisNames.length; i++) {
            document.getElementById("td_" + axisNames[i]).innerHTML = machineStatus["pos"][axisNames[i]].toFixed(2);
        }
    }
	window.RBotUIVars.statusUpdateTimer = setTimeout(statusUpdate, 100);
}

function callAjax(url, okCallback, failCallback)
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function()
	{
		if (xmlhttp.readyState === 4) {
			if (xmlhttp.status === 200) {
				if ((okCallback !== undefined) && (typeof okCallback !== 'undefined'))
					okCallback(xmlhttp.responseText);
			}
			else {
				if ((failCallback !== undefined) && (typeof failCallback !== 'undefined'))
					failCallback(xmlhttp);
			}
		}
	};

	xmlhttp.open("GET", url, true);
	xmlhttp.send();
}

function ajaxCallback()
{
	console.log("AjaxCallback");
}

function postJson(url, jsonStrToPos, okCallback, failCallback)
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function()
	{
		if (xmlhttp.readyState === 4) {
			if (xmlhttp.status === 200) {
				if ((okCallback !== undefined) && (typeof okCallback !== 'undefined'))
					okCallback(xmlhttp.responseText);
			}
			else {
				if ((failCallback !== undefined) && (typeof failCallback !== 'undefined'))
					failCallback(xmlhttp);
			}
		}
	};
	xmlhttp.open("POST", url);
	xmlhttp.setRequestHeader("Content-Type", "application/json");
	xmlhttp.send(jsonStrToPos);
}

function sendGcodeFromInput()
{
	var gcode = document.getElementById('gcodeSendBox').value;
	if (gcode.length !== 0) {
        callAjax('/exec/' + gcode, ajaxCallback);
    }
}

function moveClick(axis, dist)
{
	console.log("MoveClick " + axis  + ", " + dist);
	var axisNames = window.RBotUIVars.axisNames;
	// Check for home
	if (dist === 0) {
        callAjax("/exec/G28 " + axisNames[axis].toUpperCase());
    }
	else {
		// Set relative positioning and restore to absolute afterwards
		uploadSendFile("G91\nG0 " + axisNames[axis].toUpperCase() + dist.toString() + "\nG90");
    }
}

function fileUploadChange(e) {
    if (this.files.length > 0) {
    	startUpload(this.files[0]);
		var inputFileUpload = document.getElementById("input_file_upload");
		inputFileUpload.value = ""
    }
}

function buttonUploadClick(e) {
	var inputFileUpload = document.getElementById("input_file_upload");
	inputFileUpload.click();
    e.preventDefault();
}

function buttonUploadCancel(e) {
	clearTimeout(window.RBotUIVars.uploadRetryTimer);
	callAjax("/stop");
	setUploadState(false);
	showUploadStatus("Cancelled");
    e.preventDefault();
}

function showUploadStatus(statusStr)
{
	txtUploadStatus = document.getElementById("txtUploadStatus");
	txtUploadStatus.innerHTML = statusStr;
}

function setUploadState(uploadInProgress)
{
	window.RBotUIVars.uploadInProgress = uploadInProgress;
	btnUpload = document.getElementById("btnUpload");
	if (uploadInProgress)
		btnUpload.classList.add("disabled");
	else
		btnUpload.classList.remove("disabled");
	btnUploadCancel = document.getElementById("btnUploadCancel");
	if (!uploadInProgress)
		btnUploadCancel.classList.add("disabled");
	else
		btnUploadCancel.classList.remove("disabled");
}

function startUpload(fileToUpload) {
	// See if upload in progress
	if (window.RBotUIVars.uploadInProgress) {
		console.log("Upload still in progress");
        return;
    }
	// Check if the file is within bounds
	if (fileToUpload.size > window.RBotUIVars.MAX_FILE_SIZE) {
        alert("File is too large");
        return;
    }
	// Setup a new upload
	window.RBotUIVars.uploadFile = fileToUpload;
	window.RBotUIVars.uploadFileStartTime = new Date();
	window.RBotUIVars.uploadLines = [];
	window.RBotUIVars.uploadCurLine = 0;
	window.RBotUIVars.uploadRetryCount = 0;
	// Set status
	setUploadState(true);
	// Show status
	showUploadStatus("Uploading " + window.RBotUIVars.uploadFile.name);
	// Read the file and start sending
	uploadReadFile().then(uploadSendFile, uploadReadFailed);
}

function uploadSendFile(dataToSend) {
	// Split file into lines
	window.RBotUIVars.uploadLines = dataToSend.toString().split("\n");
	window.RBotUIVars.uploadCurLine = 0;
	window.RBotUIVars.uploadRetryCount = 0;
	uploadSendNextChunk();
	// console.log("Success");

}

function uploadReadFailed(err) {
	setUploadState(false);
	showUploadStatus("Failed to upload file" + err)
}

function isGCode(inData)
{
	for (var i = 0; i < inData.length && i < 1000;  i++)
	{
		if (inData.charCodeAt(i) > 127)
			return false;
	}
	return true;
}

function uploadReadFile() {
	return new Promise(function(resolve, reject) {
	   var reader = new FileReader();
		// Closure to capture the file information.
		reader.onload = function(theFile) {
            console.log(theFile.target);
            // Check if file is GCode
            if (isGCode(theFile.target.result)) {
                resolve(theFile.target.result);
            }
            else {
            	reject(new Error("File isn't GCode"));
			}
		};
		reader.onerror = function(err) {
			reject(err);
		};
		// Read in the image file as a data URL.
		reader.readAsText(window.RBotUIVars.uploadFile);
    });
}

function sendLineOfGCode(gCodeLine) {
	return new Promise(function(resolve,reject) {
		var xmlhttp = new XMLHttpRequest();
		xmlhttp.onreadystatechange = function()
		{
			if (xmlhttp.readyState === 4) {
				if (xmlhttp.status === 200) {
					resolve(xmlhttp.responseText);
				}
				else {
					reject(xmlhttp);
				}
			}
		};
		xmlhttp.open("GET", '/exec/' + gCodeLine, true);
		xmlhttp.send();
	});
}
function uploadSendNextChunk()
{
	// Send a line
	sendLineOfGCode(window.RBotUIVars.uploadLines[window.RBotUIVars.uploadCurLine]).then(uploadMoveNext, uploadRetry);
}

function uploadMoveNext()
{
	console.log("Sent line " + window.RBotUIVars.uploadCurLine);
	window.RBotUIVars.uploadRetryCount = 0;
	window.RBotUIVars.uploadCurLine += 1;
	while (window.RBotUIVars.uploadCurLine < window.RBotUIVars.uploadLines.length) {
		if(window.RBotUIVars.uploadLines[window.RBotUIVars.uploadCurLine].trim().length === 0) {
			window.RBotUIVars.uploadCurLine += 1;
			continue
		}
        uploadSendNextChunk();
		return;
    }
    console.log("Upload complete");
	setUploadState(false);
	showUploadStatus("")
}

function uploadRetry(xmlHttpReq)
{
	window.RBotUIVars.uploadRetryCount += 1;
	console.log("Failed to send,response " + xmlHttpReq.status.toString());
	// Check status for i'm-busy codes
	if (xmlHttpReq.status === 218 || xmlHttpReq.status === 503) {
		if (window.RBotUIVars.uploadRetryCount > window.RBotUIVars.MAX_UPLOAD_RETRIES) {
            console.log("Failed after " + window.RBotUIVars.MAX_UPLOAD_RETRIES + " retries");
        }
		else {
            window.RBotUIVars.uploadRetryTimer = setTimeout(uploadSendNextChunk, window.RBotUIVars.UPLOAD_RETRY_TIME_MS);
            return;
        }
	}
	else
	{
		console.log("Upload failed, response " + xmlHttpReq.status.toString());
	}
	// No longer uploading
	setUploadState(false);
	showUploadStatus("Upload failed")

}


