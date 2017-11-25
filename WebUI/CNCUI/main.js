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
		axisNames: ["x","y","z"]
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
		moveStr = "M120\nG91\nG1\nM121";
        callAjax("/exec/G0 " + axisNames[axis].toUpperCase() + dist.toString());
    }
}

function fileUploadChange(e) {
    if (this.files.length > 0) {
    	startUpload(this.files[0]);
		var inputFileUpload = document.getElementById("input_file_upload");
		inputFileUpload.value = ""
        // For POST uploads we need file blobs
        // startUpload($(this).data("type"), this.files, false);
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

	// // Initialize some values
	// stopUpdates();
	// isUploading = true;
	// uploadType = type;
	// uploadTotalBytes = fileToUpload.size;
    //
	// // Prepare some upload values
	// uploadFileName = fileToUpload.name;
	// uploadFileSize = fileToUpload.size;
	// uploadStartTime = new Date();
	// uploadPosition = 0;

	// // Update the GUI
	// uploadRows[0].find(".progress-bar > span").text(T("Starting"));
	// uploadRows[0].find(".glyphicon").removeClass("glyphicon-asterisk").addClass("glyphicon-cloud-upload");

	// // Begin another POST file upload
	// uploadRequest = $.ajax(ajaxPrefix + "rr_upload?name=" + encodeURIComponent(targetPath) + "&time=" + encodeURIComponent(timeToStr(new Date(file.lastModified))), {
	// 	data: file,
	// 	dataType: "json",
	// 	processData: false,
	// 	contentType: false,
	// 	timeout: 0,
	// 	type: "POST",
	// 	global: false,
	// 	error: function(jqXHR, textStatus, errorThrown) {
	// 		finishCurrentUpload(false);
	// 	},
	// 	success: function(data) {
	// 		if (isUploading) {
	// 			finishCurrentUpload(data.err == 0);
	// 		}
	// 	},
	// 	xhr: function() {
	// 		var xhr = new window.XMLHttpRequest();
	// 		xhr.upload.addEventListener("progress", function(event) {
	// 			if (isUploading && event.lengthComputable) {
	// 				// Calculate current upload speed (Date is based on milliseconds)
	// 				uploadSpeed = event.loaded / (((new Date()) - uploadStartTime) / 1000);
    //
	// 				// Update global progress
	// 				uploadedTotalBytes += (event.loaded - uploadPosition);
	// 				uploadPosition = event.loaded;
    //
	// 				var uploadTitle = T("Uploading File(s), {0}% Complete", ((uploadedTotalBytes / uploadTotalBytes) * 100).toFixed(0));
	// 				if (uploadSpeed > 0) {
	// 					uploadTitle += " (" + formatUploadSpeed(uploadSpeed) + ")";
	// 				}
	// 				$("#modal_upload h4").text(uploadTitle);
    //
	// 				// Update progress bar
	// 				var progress = ((event.loaded / event.total) * 100).toFixed(0);
	// 				uploadRows[0].find(".progress-bar").css("width", progress + "%");
	// 				uploadRows[0].find(".progress-bar > span").text(progress + " %");
	// 			}
	// 		}, false);
	// 		return xhr;
	// 	}
	// });
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
//
// function uploadReadFile2() {
// 	return new Promise(function(resolve, reject) {
// 		var reader = FileReader();
// 		reader.onloadend = function(evt) {
// 			if (evt.target.readyState === FileReader.DONE) { // DONE === 2
// 				console.log("Read file " + evt.target.name);
// 				window.RBotUIVars.uploadChunk = evt.target.result;
// 				window.RBotUIVars.uploadChunkPos = 0;
// 				window.RBotUIVars.uploadChunkLen = evt.target.result.length;
// 				resolve();
// 			}
// 		};
// 		reader.onerror = function(err) {
// 			reject(err);
// 		};
// 		reader.readAsText(window.RBotUIVars.uploadFile);
// 	});
// }

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

// function getChunkFromFile(file, opt_startPos, opt_maxLen, callbackOnComplete) {
//     var start = parseInt(opt_startPos) || 0;
//     var blobLen = parseInt(opt_maxLen) || file.size - start;
//     var reader = new FileReader();
//     reader.onloadend = function(evt) {
//         if (evt.target.readyState === FileReader.DONE) { // DONE === 2
// 			console.log("got " + evt.target.result);
// 			window.RBotUIVars.uploadChunk = evt.target.result;
// 			window.RBotUIVars.uploadChunkPos = 0;
// 			window.RBotUIVars.uploadChunkLen = evt.target.result.length;
// 			callbackOnComplete();
//         }
//     };
//     var blob = file.slice(start, start+blobLen);
// 	reader.readAsBinaryString(blob);
// }

	// // Read a chunk from the file
	// getChunkFromFile(window.RBotUIVars.uploadFile,
   	// 		window.RBotUIVars.uploadFilePos, window.RBotUIVars.MAX_POST_CHUNK_SIZE);
    //

// function uploadGetLineFromFile(callbackWhenDone)
// {
// 	window.RBotUIVars.uploadLine = "";
// 	while (window.RBotUIVars.uploadChunkPos < window.RBotUIVars.uploadChunkLen)
// 	{
//
// 	}
// }

// function uploadGetChunkAndSend() {
// 	// Get line from the file
// 	uploadGetLineFromFile()
// }
//
   //
   // var reader = new FileReader();
   //  // Closure to capture the file information.
   //  reader.onload = function(theFile) {
   //  	console.log(theFile.target.result);
   //      return theFile;
   //    };
   //
   //    // Read in the image file as a data URL.
   //    reader.readAsText(window.RBotUIVars.uploadFile);

	// 	var http = new XMLHttpRequest();
// var url = "get_data.php";
// var params = "lorem=ipsum&name=binny";
// http.open("POST", url, true);
//
// //Send the proper header information along with the request
// http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
//
// http.onreadystatechange = function() {//Call a function when the state changes.
//     if(http.readyState == 4 && http.status == 200) {
//         alert(http.responseText);
//     }
// }
// http.send(params);
// }