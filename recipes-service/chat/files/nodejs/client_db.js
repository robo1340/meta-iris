
let msg_db = undefined;
let user_db = undefined;
let settings_db = undefined;

let my_username = undefined;

var socket;

var location_watch_id = null;
var notifications = false;

function do_nothing(data) {
	//console.log('donothing');
	return;
}

/*
function get_iso_timestamp(){
	var date = new Date();
	return date.toISOString()
}
*/

//function get_iso_timestamp(){
//	var date = new Date();
//	return date.toISOString()
//}


function get_iso_timestamp(date) {
	var date = new Date();
    return date.toLocaleString('sv').replace(' ', 'T');
}

////////////////////////////////////////////////////////////////
////////////////////Database Operations/////////////////////////
////////////////////////////////////////////////////////////////

function addUser(username, lat=null, lon=null){
	if (user_db === undefined){
		return
	}

	var last_activity_time = get_iso_timestamp(); //"2011-12-19T15:28:46.493Z"
	var last_location_beacon_time = null;
	if ((lat !== null) && (lon !== null)){
		last_location_beacon_time = last_activity_time
	}
	
	const transaction = user_db.transaction(["notes_os"], "readwrite");
	const objectStore = transaction.objectStore("notes_os");
	const addRequest = objectStore.add({
		username:username, 
		lat:lat, lon:lon, 
		last_location_beacon_time:last_location_beacon_time, 
		last_activity_time:last_activity_time
	});		
}

function get_user_object_store(){
	return user_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
}


function getUser(username, cb) {
	if (user_db === undefined) {cb(null);}
	const read_request = get_user_object_store().get(username);
	read_request.onsuccess = function() {
		if (read_request.result === undefined){cb(null);}
		return cb(read_request.result);
    };	
}

function updateUser(username, coords=null, timestamp, complete_cb=do_nothing){
	if (user_db === undefined){
		return;
	}
	if (timestamp == null){
		timestamp = get_iso_timestamp();
	}
	
	const read_request = get_user_object_store().get(username);
	read_request.onsuccess = function() {
		if (read_request.result === undefined){
			if (coords !== null){
				addUser(username, coords[0], coords[1]);
			} else {
				addUser(username);
			}
		} else {
			//console.log(read_request.result)
			var last_activity_time = timestamp; //"2011-12-19T15:28:46.493Z"
			var last_location_beacon_time = null;
			var new_lat = read_request.result.lat;
			var new_lon = read_request.result.lon;
			if (coords !== null){
				last_location_beacon_time = last_activity_time;
				new_lat = coords[0];
				new_lon = coords[1];
			}
			
			const transaction = user_db.transaction(["notes_os"], "readwrite");
			const objectStore = transaction.objectStore("notes_os");
			const updated_user = {
				username:username, 
				lat:new_lat, lon:new_lon, 
				last_location_beacon_time:last_location_beacon_time, 
				last_activity_time:last_activity_time
			};
			const addRequest = objectStore.put(updated_user);
			complete_cb(updated_user);
		}
    };
	
}

function addMessage(username, msg) {
	console.log("addMessage");
	if (msg_db === undefined){
		console.log("addMessage() failed msg_db is undefined");
		return
	}
	//console.log(msg);
	const transaction = msg_db.transaction(["notes_os"], "readwrite");
	const objectStore = transaction.objectStore("notes_os");
	const addRequest = objectStore.add({username:username, msg:msg, time:get_iso_timestamp()});	
}

/*
function setup_settings_db(cb=do_nothing){
	const settings_open_req  = window.indexedDB.open("settings_db", 1);
	
	settings_open_req.addEventListener("error", () =>
		console.error("msg_open_req failed to open"),
	);
	
	settings_open_req.addEventListener("success", () => {
		settings_db = msg_open_req.result;
		cb();
	});
	
	settings_open_req.addEventListener("upgradeneeded", (e) => {
		settings_db = e.target.result;

		// Create an objectStore in our database to store notes and an auto-incrementing key
		// An objectStore is similar to a 'table' in a relational database
		const objectStore = settings_db.createObjectStore("notes_os", {
			keyPath: "id",
			autoIncrement: true,
		});

		// Define what data items the objectStore will contain
		objectStore.createIndex("username", "username", {unique: false });
		objectStore.createIndex("msg", "msg", 			{unique: false });
		objectStore.createIndex("time", "time", 		{unique: false });

		console.log("Database setup complete");
		cb();
	});	
}
*/

function setup_db(msg_db_cb=do_nothing,user_db_cb=do_nothing) {
	const msg_open_req  = window.indexedDB.open("msg_db", 1);
	const user_open_req = window.indexedDB.open("user_db", 1);
	
	msg_open_req.addEventListener("error", () =>
		console.error("msg_open_req failed to open"),
	);
	
	user_open_req.addEventListener("error", () =>
		console.error("user_open_req failed to open"),
	);
	
	msg_open_req.addEventListener("success", () => {
		msg_db = msg_open_req.result;
		msg_db_cb();
	});

	user_open_req.addEventListener("success", () => {
		user_db = user_open_req.result;
		user_db_cb();
	});
	
	msg_open_req.addEventListener("upgradeneeded", (e) => {
		console.log("Upgrade msg_db");
		msg_db = e.target.result;

		// Create an objectStore in our database to store notes and an auto-incrementing key
		// An objectStore is similar to a 'table' in a relational database
		const objectStore = msg_db.createObjectStore("notes_os", {
			keyPath: "id",
			autoIncrement: true,
		});

		// Define what data items the objectStore will contain
		objectStore.createIndex("username", "username", {unique: false });
		objectStore.createIndex("msg", "msg", 			{unique: false });
		objectStore.createIndex("time", "time", 		{unique: false });

		console.log("Database setup complete");
	});
	
	user_open_req.addEventListener("upgradeneeded", (e) => {
		console.log("upgrade user_db");
		user_db = e.target.result;

		// Create an objectStore in our database to store notes and an auto-incrementing key
		// An objectStore is similar to a 'table' in a relational database
		const objectStore = user_db.createObjectStore("notes_os", {
			keyPath: "username",
			//keyPath: "id",
			autoIncrement: true,
		});

		// Define what data items the objectStore will contain
		objectStore.createIndex("username", "username", {unique: false });
		objectStore.createIndex("lat", "lat", 			{unique: false });
		objectStore.createIndex("lon", "lon", 			{unique: false });
		objectStore.createIndex("last_location_beacon_time", "last_location_beacon_time", 	{unique: false });
		objectStore.createIndex("last_activity_time", "last_activity_time", 				{unique: false });
		console.log("Database setup complete");
	});
}

function load_messages(finished_cb=do_nothing) {
	if (msg_db !== undefined){
		const transaction = msg_db.transaction(["notes_os"], "readwrite");
		const objectStore = transaction.objectStore("notes_os");
		var messages = objectStore.getAll();
		messages.onsuccess = function() {
			finished_cb(messages.result);
		};
	}
}

function clear_messages(finished_cb=do_nothing) {
	if (msg_db === undefined){return;}
	const objectStore = msg_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
	objectStore.clear();
	finished_cb();
}

function load_users(finished_cb=do_nothing) {
	if (user_db === undefined){return;}
	const objectStore = user_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
	var users = objectStore.getAll();
	users.onsuccess = function() {
		finished_cb(users.result);
	};
}

function clear_users(finished_cb=do_nothing) {
	if (user_db === undefined){return;}	
	const objectStore = user_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
	objectStore.clear();
	finished_cb();
}

function delete_user(username, finished_cb=do_nothing) {
	if (user_db === undefined){return;}	
	get_user_object_store().delete(username); 
	finished_cb();
}

////////////////////////////////////////////////////////////////
///////////////////////LOCATION RELATED/////////////////////////
////////////////////////////////////////////////////////////////

var my_location = null
const MAX_LOCATION_TX = 5000;
var last_location_tx = -1;

const position_received_cb = (position) => {
	var coords = [position.coords.latitude,position.coords.longitude];

	my_location = {'username':my_username, 'coords':coords};
	messenger.postMessage(["my_location",my_location]);
	//console.log(my_location);
	updateUser(my_username, my_location.coords);
	
	if (first_map_update == false){
		first_map_update = true;
		set_map_location(coords);
	} else {
		markers[my_username].setLatLng(coords);
	}
}

const DEFAULT_COORDS = [37.54557, -97.26893];

const position_error_cb = (error) => {
	if (first_map_update == false){
		set_map_location(DEFAULT_COORDS);
		first_map_update = true;
		var popup = L.popup(DEFAULT_COORDS, options={"class" : "map_context_popup"})
		.setContent("Location of Local Device Not Found, the map was set to a default location")
		.openOn(map);  
	}
	console.log(error);
}

function setup_location_watch(rx_cb=position_received_cb, error_cb=position_error_cb) {
	location_watch_id = navigator.geolocation.watchPosition(
		rx_cb, 
		error_cb, 
		{
			enableHighAccuracy:true,
			maximumAge:30000,
			timeout: 15000
		}
	);
	//navigator.geolocation.clearWatch(watchID);
}

function setup_notifications(){
	if (!("Notification" in window)) {
		console.log("This browser does not support desktop notification");
	}
	else if (Notification.permission === "granted"){
		console.log("Notifications granted")
		notifications = true;
	}
	else if (Notification.permission !== "denied") {
		Notification.requestPermission().then((permission) => {
			notifications = true;
		});
	} else {
		console.log("Notifications denied")
	}
}

var location_beacon_ms = null;

////////////////////////////////////////////////////////////////
////////////////////////////UTILITY ////////////////////////////
////////////////////////////////////////////////////////////////

function iso_to_time(iso_str){
	try {
		return iso_str.split('T')[1].split('.')[0];
	} catch {return "";}
}

function iso_to_date(iso_str) {
	try {
		return iso_str.split('T')[0];
	} catch {return "";}
}