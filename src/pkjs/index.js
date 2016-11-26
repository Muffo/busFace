
var xhrRequest = function (url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send();
};


function getTimestamp() {
	var milliseconds = new Date().getTime();
	return Math.floor(milliseconds / 1000);
}


function secondsToTime(secs) {
	var hours = Math.floor(secs / (60 * 60));
	
	var divisor_for_minutes = secs % (60 * 60);
	var minutes = Math.floor(divisor_for_minutes / 60);
	
	var divisor_for_seconds = divisor_for_minutes % 60;
	var seconds = Math.ceil(divisor_for_seconds);
	
	var obj = {
	 	"h": hours,
	 	"m": minutes,
	 	"s": seconds
	};
	return obj;
}
function timeString(timeDiff) {
	var due;
	var time = secondsToTime(timeDiff);

	if (timeDiff <= 60) {
		due = '1 min';
	}
	else if (timeDiff < 3600) {
		if(time.m == 1) {
		  	due = '1 min';
		}
		else {
		  	due = time.m + ' mins';
		}
	}
	else if (time.h == 1) {
		if (time.m == 1) {
		 	due = time.h + ' hour ' + time.m + ' min';
		}
		else {
		  due = time.h + ' hour ' + time.m + ' mins';
		}
	}
	else {
		if(time.m == 1) {
		  	due = time.h + ' hours ' + time.m + ' min';
		}
		else {
		 	 due = time.h + ' hours ' + time.m + ' mins';
		}
	}
	return due;
}

function updateBusTime() {
	var url = "http://www.buseireann.ie/inc/proto/stopPassageTdi.php?stop_point=6350786630982641495&_=1446809752646"


	xhrRequest(url, 'GET', 
		function(responseText) {
			var data = JSON.parse(responseText);
			var now = getTimestamp();
			var nextPassage = {
		      	waitTime: 999999
		    };
      
		    for (var key in data['stopPassageTdi']) {
		        
		        if (key === "foo")
		          continue;
		        
		        var passage = data['stopPassageTdi'][key];
		        var departure = passage.departure_data;
		        var destination = departure.multilingual_direction_text.defaultValue;
		        
		        // Ignore all but Limerick buses
		        if (destination !== "Limerick")
		        	continue;
		        
		        passage.actual = departure.actual_passage_time_utc;
		        passage.scheduled = departure.scheduled_passage_time_utc;

		        if (passage.actual)
		          passage.waitTime = parseInt(passage.actual - now);
		        else
		          passage.waitTime = parseInt(passage.scheduled - now);

		        // Ignore passages that happened more than 3 minutes ago
		        if (passage.waitTime < -180 || passage.waitTime > nextPassage.waitTime)
		          continue;
		        
		        passage.waitTimeString = timeString(passage.waitTime);
		        
		        console.log("Passage for Limerick due in  " + passage.waitTimeString + " (" + passage.waitTime + ")");
		        nextPassage = passage;
		    }
		    		    
		    if (!nextPassage.waitTimeString) {
		      console.log("Failed to get info from BusEireann on next passage. See data below");
		      console.log(JSON.stringify(data));
		      return;
		    }
		    		    
	        // Assemble dictionary using our keys
			var busData = {
				'SCHEDULED': nextPassage.actual ? 0 : 1,
				'DEPARTED': nextPassage.waitTime > 0 ? 1 : 0,
				'DUEIN': nextPassage.waitTimeString,
			};

			// Send to Pebble
			Pebble.sendAppMessage(busData,
				function(e) {
					console.log('Bus info sent to Pebble successfully:');
					console.log(JSON.stringify(busData));
				},
				function(e) {
					console.log('Error sending bus info to Pebble!');
				}
			);
		}      
	);
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
	function(e) {
		console.log('PebbleKit JS ready!');
	    updateBusTime();
	}
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
	function(e) {
		console.log('AppMessage received!');
		updateBusTime();
	}                     
);