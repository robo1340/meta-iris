
const COOKIE_EXP=30

function add_saved_message(username, msg){
	addMessage(username, msg);
	add_unsaved_message(username, msg);
}

var msg_last_grey = false; //Used to alternate color of messages;

function add_unsaved_message(username, msg){
    var li = $("<li></li>",{
			"class": "message",
			"style": "background-color:" + (msg_last_grey ? "white" : "#eee") + ";list-style-type:none;",
			"text": username + ": " + msg
		});
    $("#list").append(li);
    msg_last_grey = !msg_last_grey;
    //$('input[name=messageText]').val("");
    
    //scroll to bottom of div with scroll bar
    //setting position:fixed for this div is important
    //in adding a scroll bar to it
	$("#list").scrollTop($("#list")[0].scrollHeight);


	//$("#list").scrollTop($("#list").height());
	//$("#chatBox").scrollTop($("#chatBox").height());
	//$("div").scrollTop($("div").height());		
}

function send_msg() {
	try {
		console.log("send_msg()");
		var msg = $('input[name=messageText]').val()
		//console.log(msg)
		
		messenger.postMessage(["newMessage",{'username': my_username, 'message': msg}]);
		add_saved_message(my_username, msg);
		$("input[name=messageText]").val(''); //clear the text input
	}
	catch (error) {console.log(error);}
	return false; //very important to not get redirected
}

function clear_messages_ui() {
	try {
		clear_messages(); //delete all messages from the db
		$("#list").empty();
	}
	catch (error) {console.log(error);}
	return false;
}
function clear_users_ui() {
	try {
		clear_users(); //delete all messages from the db
		$("#userList").empty();
		for (const [k, m] of Object.entries(markers)) {
			if (k == my_username){continue;}
			m.remove();
		}
	} catch(error){
		console.log(error);
	}
	return false;
}
function clear_waypoints_ui() {
	try {
		clear_waypoints(); //delete all messages from the db
		for (const [k, w] of Object.entries(waypoints)) {
			w.remove();
		}
	} catch(error){
		console.log(error);
	}
	return false;
}

var selected_radio_config = null;
function set_radio_config(config_type='modem') {
	console.log("set_radio_config("+config_type+")");
	var selected = '';
	if (config_type=='modem'){
		selected = selected_radio_config;
	} else {
		selected = selected_radio_config_other[config_type];
	}
	
	if ((selected === null)||(selected === undefined)){return;} //do nothing
	messenger.postMessage(["select_radio_config",{'config_type':config_type,'selected':selected}]);
	lbl = document.getElementById('current_radio_config');
	lbl.innerHTML = "Current: LOADING";
	lbl = document.getElementById('current_radio_general_config');
	lbl.innerHTML = "Current: LOADING";
	lbl = document.getElementById('current_radio_packet_config');
	lbl.innerHTML = "Current: LOADING";
	lbl = document.getElementById('current_radio_preamble_config');
	lbl.innerHTML = "Current: LOADING";
	
	setTimeout(function(){
		get_current_radio_config()	
		get_current_radio_config_other()
	}, 2500);
}

function set_radio_modem_config() {set_radio_config(config_type='modem');};
function set_radio_general_config() {set_radio_config(config_type='general');};
function set_radio_packet_config() 	{set_radio_config(config_type='packet');};
function set_radio_preamble_config(){set_radio_config(config_type='preamble');};

function setup_button_callbacks(){

	document.getElementById("send_button").onclick = get_message_to_send;
	//$("#messageInput").submit(send_msg);
	
	document.getElementById("clear_messages").onclick = clear_messages_ui;
	document.getElementById("clear_users").onclick = clear_users_ui;
	document.getElementById("clear_waypoints").onclick = clear_waypoints_ui;
	document.getElementById("set_radio_config").onclick = set_radio_modem_config;
	document.getElementById("set_radio_general_config").onclick = set_radio_general_config;
	document.getElementById("set_radio_packet_config").onclick = set_radio_packet_config;
	document.getElementById("set_radio_preamble_config").onclick = set_radio_preamble_config;
	document.getElementById("set_username").onclick = set_username;
	document.getElementById("do_nothing").onclick = function() {return false;};
	document.getElementById("radio_configs").onclick = load_radio_configs;
	document.getElementById("radio_general_configs").onclick = load_radio_general_configs;
	document.getElementById("radio_packet_configs").onclick = load_radio_packet_configs;
	document.getElementById("radio_preamble_configs").onclick = load_radio_preamble_configs;
	
	
	/*
	$(document).keypress(function(event) {
		var keycode = event.keyCode || event.which;
		if(keycode == '13') {
			//console.log('You pressed a "enter" key in somewhere');
			send_msg();
		}
	});
	*/
}

function other_user_message_cb(data, timestamp){
	console.log("other_user_message_cb");
	if (data.username == my_username){return;}
	add_saved_message(data.username, data.message);
	if (notifications == true){
		const notification = new Notification(data.username + ": " + data.message);
	}
	set_tab_pending("chat");
}

var markers = {}; //a dictionary of location markers for other users

function generate_user_tooltip_contents(username, last_location_beacon_time=null, coords=null){
	if (username == my_username){
		return '(Me) ' + username  + ' ' + coords;
	} else {
		return username + ' ' + iso_to_time(last_location_beacon_time) + ' ' + coords;
	}
	return to_return;
}

function other_user_location_cb(data, timestamp){
	if (data == null){return;}
	if (data.username == my_username){return;}
	//console.log("other_user_location_cb");
	//console.log(data);
	updateUser(data.username, data.coords, timestamp, type=data.type, function(user){
		if (first_map_update == false){
			first_map_update = true;
			set_map_location(data.coords);
		}
		if (user.username in markers){
			markers[data.username].setLatLng(data.coords);
			markers[data.username]._tooltip._content = generate_user_tooltip_contents(data.username, data.last_location_beacon_time, data.coords);
			
			let e = $('#'+user.username+'.user-entry-time');
			e.html(iso_to_time(user.last_location_beacon_time));
			//console.log($("#userList")[0]);
		} else {
			const i = Object.keys(markers).length;
			var user_icon = user_icon_factory(i, data.type);
			markers[data.username] = L.marker(data.coords, {icon: user_icon}).addTo(map);
			markers[data.username].bindTooltip(generate_user_tooltip_contents(user.username, user.last_location_beacon_time, data.coords),
				{permanent: false, direction: 'right', className: 'leaflet-tooltip'}
			);
			add_user_to_list(user, i, me=false, stale=false, type=data.type);
		}
	});
}

function set_tab_pending(tabname){
	try {
		if (document.getElementById(tabname).style.display == "none"){
			if (document.getElementById(tabname+"_button").className.includes("pending") != true){
				document.getElementById(tabname+"_button").className += " pending";
			}
		}
	} catch(error) {console.log(error);}
}

///////////////////////// Loading Users and Messages on load///////////////////////////////////////

function go_to_user(username) {
	const marker = markers[username]
	if (marker === undefined) {return;}
	var popup = L.popup({className: "leaflet-tooltip"}).setLatLng(marker._latlng).setContent('<p>'+username+'</p>').openOn(map);
	setTimeout(function(){popup.close()}, 1500);
}

function go_to_waypoint(username) {
	const w = waypoints[username]
	if (w === undefined) {return;}
	var popup = L.popup({className: "leaflet-tooltip"}).setLatLng(w._latlng).setContent('<p>'+w._tooltip._content+'</p>').openOn(map);
	setTimeout(function(){popup.close()}, 1500);
}

var user_last_grey = false;

function add_user_to_list(user, index, me=false, stale=false, type=null){
	
	var color = (me == false) ? user_color_by_id(index) : "ME";
	//console.log(index + ' ' + color);
	var time  =  ((stale == true)&&(me==false)) ?  "INACTIVE " : iso_to_time(user.last_location_beacon_time);
	
    var li = $("<tr></tr>",{
			"class": "user-entry",
			"id" : user.username,
			"style": "background-color:" + (user_last_grey ? "white" : "#eee") + ";list-style-type:none;",
		});
	var span_id = $("<td></td>",{
			"class": "user-entry-index " + color,
			"id" : user.username,
			"text" : index + '.'
		});
	var span_name = $("<td></td>",{
			"class": "user-entry-name",
			"id" : user.username,
			"text" : user.username
		});
	var span_time = $("<td></td>",{
			"class": "user-entry-time",
			"id" : user.username,
			"text" : time
		});
	var span_waypoint = $("<td></td>",{
			"class": "user-entry-waypoint",
			"id" : user.username,
			"text" : "WAYPOINT"
		});
	
	li.append(span_id);
	li.append(span_name);
	li.append(span_time);
	li.append(span_waypoint);
    $("#userList").append(li);
    user_last_grey = !user_last_grey;

	$("#"+user.username+" .user-entry-name").click(function(e) {
		go_to_user(e.currentTarget.id);
	});
	
	$("#"+user.username+" .user-entry-time").click(function(e) {
		go_to_user(e.currentTarget.id);
	});
	
	//$("#"+user.username+"_0 td").click(function(e){go_to_user(e.currentTarget.id.split("_")[0])});
	//$("#"+user.username+"_1").click(function(e){go_to_user(e.currentTarget.id)});
	//$("#"+user.username+"_2").click(function(e){go_to_user(e.currentTarget.id)});
	
	$("#"+user.username+" .user-entry-waypoint").click(function(e) {
		go_to_waypoint(e.currentTarget.id);
	});
	
    //scroll to bottom of div with scroll bar
    //setting position:fixed for this div is important
    //in adding a scroll bar to it
    $("#userPane").scrollTop($("#userPane")[0].scrollHeight);		
}

var user_colors = {
	0 : "Crimson",
	1 : "DarkCyan",
	2 : "Chocolate",
	3 : "Chartreuse",
	4 : "Magenta",
	5 : "DarkOrange",
	6 : "ForestGreen",
	7 : "Gold",
	8 : "Aqua"
}
var num_user_colors = Object.keys(user_colors).length;

function user_color_by_id(id){
	return user_colors[(id)%num_user_colors];
}

const HTML_TRIANGLE_1 = `
			<svg
			  width="24"
			  height="36"
			  viewBox="0 0 100 100"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M0 0 L50 100 L100 0 Z" fill="#7A8BE7"></path>
			</svg>`;

const HTML_CIRCLE_1 = `
			<svg
			  width="26"
			  height="26"
			  viewBox="0 0 100 100"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<circle r="45" cx="50" cy="50" fill="red" /></circle>
			</svg>`;
			
const HTML_CIRCLE_2 = `
			<svg
			  width="14"
			  height="14"
			  viewBox="0 0 100 100"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<circle r="45" cx="50" cy="50" fill="#7A8BE7" /></circle>
			</svg>`;
			
const ICON_BY_USER_TYPE = {
	'station' : {'html' : HTML_CIRCLE_2, 'size': [14, 14],'anchor' : [7, 7]},
	'self' : {'html' : HTML_CIRCLE_1, 'size': [24, 36],'anchor' : [12, 36]},
	null : {'html' : HTML_TRIANGLE_1, 'size': [24, 36],'anchor' : [12, 36]},
	undefined : {'html' : HTML_TRIANGLE_1, 'size': [24, 36],'anchor' : [12, 36]},
};

function user_icon_factory(id, type=null){
	//console.log(type);
	var class_name = "user_icon " + user_color_by_id(id);
	var to_return = L.divIcon({
		html: ICON_BY_USER_TYPE[type].html,
		className: class_name,
		iconSize: ICON_BY_USER_TYPE[type].size,
		iconAnchor: ICON_BY_USER_TYPE[type].anchor
	});	
	return to_return;
}

function circle_icon_factory(){
	const class_name = "circle_icon";
	var to_return = L.divIcon({
		html: `
			<svg
			  width="26"
			  height="26"
			  viewBox="0 0 100 100"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<circle r="45" cx="50" cy="50" fill="red" /></circle>
			</svg>`,
		className: class_name,
		iconSize: [26, 26],
		iconAnchor: [13, 13]
	});	
	return to_return;
}

function users_loaded_cb(users){
	//console.log(users)
	for (i in users) {
		if (users[i].username == my_username){ 
			add_user_to_list(users[i], i, me=true, stale=true, type=users[i].type);
		}
		else {
			var user_icon = user_icon_factory(i, users[i].type);
			if ((users[i].lat !== undefined) && (users[i].lon !== undefined)) {
				var coords = [users[i].lat, users[i].lon]
				markers[users[i].username] = L.marker(coords, {icon: user_icon}).addTo(map);
				markers[users[i].username].bindTooltip( generate_user_tooltip_contents(users[i].username, users[i].last_location_beacon_time, coords),
					{permanent: false, direction: 'right', className: 'leaflet-tooltip'}
				);
			}
		
			add_user_to_list(users[i], i, me=false, stale=true, type=users[i].type);
		}
	}
	
}

var default_username = true;

function load_complete_cb(messages) {
	for (i in messages) {
		add_unsaved_message(messages[i].username, messages[i].msg);
	}
	
	setTimeout(function(){
		if (default_username == true){
			add_unsaved_message('Iris', "Welcome! Your temporary username is " + my_username + " To change it navigate to \"Settings\"");
		} else {
			add_unsaved_message('Iris', "Welcome Back \"" + my_username + "\"");
			const notification = new Notification("Welcome Back: " + my_username);
		}	
	}, 500);
	
}

function msg_db_cb() {
	load_messages(load_complete_cb);
}

function user_db_cb() {
	load_users(users_loaded_cb);
}

///////////////////////////////MAP///////////////////////////////////////////////

let map;
var first_map_update = false;

function set_map_location(lat_lon_arr){
	//console.log(lat_lon_arr)
	map.setView(lat_lon_arr, 14);
	map.flyTo(lat_lon_arr, 14);	
	
	var my_location_marker = L.marker(lat_lon_arr, {icon: circle_icon_factory()}).addTo(map);
	my_location_marker.bindTooltip(generate_user_tooltip_contents(my_username,null,lat_lon_arr), {permanent: false});
	markers[my_username] = my_location_marker;
}

//////////////////////////////////////////////////////////////////////////////

function open_tab(target) {
	var i, tabcontent, tablinks;

	// Get all elements with class="tabcontent" and hide them
	tabcontent = document.getElementsByClassName("tabcontent");
	for (i=0; i < tabcontent.length; i++) {
		tabcontent[i].style.display = "none";
	}

	// Get all elements with class="tablinks" and remove the class "active"
	tablinks = document.getElementsByClassName("tablinks");
	for (i=0; i < tablinks.length; i++) {
		if (tablinks[i].id == target){
			tablinks[i].className += " active";
			tablinks[i].className = tablinks[i].className.replace(" pending", "");
		} else {
			tablinks[i].className = tablinks[i].className.replace(" active", "");
		}
	}
	target = target.split('_')[0]
	document.getElementById(target).style.display = "block";
	if (target == "map"){
		map.invalidateSize()
	}
}


$("input[name=load_on_start]").click(function(){
	//console.log($(this).val());
	Cookies.set('load_on_start',$(this).val(), {expires:COOKIE_EXP})
});

function get_message_to_send() {
	try {
		let msg = prompt("Message:","");
		if ((msg !== null)&&(msg != "")){
			console.log("get_message_to_send()");
			messenger.postMessage(["newMessage",{'username': my_username, 'message': msg}]);
			add_saved_message(my_username, msg);			
		}
	}
	catch (error) {console.log(error);}
	return false; //very important to not get redirected
}

adjectives = {
	0 : "shady",
	1 : "quirky",
	2 : "smiley",
	3 : "tipsy",
	4 : "filthy",
	5 : "sleepy",
	6 : "rapey"
}

animals = {
	0 : "monkey",
	1 : "otter",
	2 : "hippo",
	3 : "elephant",
	4 : "horse",
	5 : "dog",
	6 : "otter"
}

function rand_int(min, max) {
	const minCeiled = Math.ceil(min);
	const maxFloored = Math.floor(max);
	return Math.floor(Math.random() * (maxFloored - minCeiled) + minCeiled); // The maximum is exclusive and the minimum is inclusive
}

function generate_username(){
	return adjectives[rand_int(0,7)] + "_" + animals[rand_int(0,7)]
}

function set_username(){
	try {
		let new_username = prompt("Enter Username","");
		if ((new_username !== null)&&(new_username != "")){
			var old = my_username;
			my_username = new_username;
			Cookies.set('username',my_username, {expires:COOKIE_EXP});
			
			if (old != my_username) {
				Object.defineProperty(markers, my_username, Object.getOwnPropertyDescriptor(markers, old));
				delete markers[old];
				markers[my_username]._tooltip._content = generate_user_tooltip_contents(my_username);
				
				getUser(old, function(old_user) {
					updateUser(my_username, coords=[old_user.lat,old_user.lon], timestamp=null);
					delete_user(old, function() {
						let e = $('#'+old+' .user-entry-name');
						e.text(my_username);
						e.attr("id",my_username);
						//console.log(e);
						messenger.postMessage(["username",my_username]);
						messenger.postMessage(["newMessage",{'username': "Iris", 'message': old + " changed username to " + my_username}]);
					});
				});
				
				
			}
		}
		document.getElementById("username_label").innerHTML = "Username: " + my_username;
	}
	catch (error) {console.log(error);}
	return false;
}

var radio_configs = null;
var radio_configs_other = {};
selected_radio_config_other = {};

function load_radio_configs(){
	console.log("load_radio_configs");
	if (radio_configs === null){
		messenger.postMessage(["get_radio_configs",{'type':'modem'}]);
	} else {
		var rs = document.getElementById("radio_configs");
		selected_radio_config = rs.options[rs.selectedIndex].value;
		//console.log(selected);
	}
}

function load_radio_other_configs(config_type){
	console.log("load_radio_other_configs("+config_type+")");
	if (!(config_type in radio_configs_other)) {
		messenger.postMessage(["get_radio_configs",{'type':config_type}]);
	} else {
		var rs = document.getElementById("radio_"+config_type+"_configs");
		selected_radio_config_other[config_type] = rs.options[rs.selectedIndex].value;
	}
}

function load_radio_general_configs(){load_radio_other_configs("general");}
function load_radio_packet_configs(){load_radio_other_configs("packet");}
function load_radio_preamble_configs(){load_radio_other_configs("preamble");}

function get_current_radio_config(){
	console.log("get_current_radio_config")
	messenger.postMessage(["get_current_radio_config",{'type':'modem'}]);
}

function get_current_radio_config_other(){
	console.log("get_current_radio_config_other")
	messenger.postMessage(["get_current_radio_config_other",{'type':'general'}]);
}

function radio_config_path_extract(path){
	const freq_re 	 = new RegExp("(\\d+)M", "gm");
	const bitrate_re = new RegExp("(\\d+)kbps", "gm");
	var freq = freq_re.exec(path);
	var bitrate = bitrate_re.exec(path);
	if ((freq === null) || (bitrate === null)){
		console.log("throwing out " + path);
		return null;
	}
	return {"freq":freq[1],"bitrate":bitrate[1],"path":path};
}

function radio_configs_other_received(config_type, config_paths){
	console.log('radio_configs_other_received('+config_type+')');
	radio_configs_other[config_type] = config_paths
	if (radio_configs_other[config_type].length == 0){
		radio_configs_other[config_type] = null;
		return;
	}
	//console.log(radio_configs_other[config_type]);
	for (i in radio_configs_other[config_type]){ //generate HTML objects now
		rs = document.getElementById('radio_'+config_type+'_configs');
		rs.add(new Option(radio_configs_other[config_type][i]));
	}
}

function radio_configs_received(value){
	config_paths = value['config_paths']
	config_type = value['config_type']
	console.log(config_paths);
	
	if (config_type == 'modem'){
		radio_configs = [];
		for (i in config_paths){
			var c = radio_config_path_extract(config_paths[i]);
			if (c === null) {continue;}
			radio_configs.push(c);
		}
		if (radio_configs.length == 0){
			radio_configs = null;
			return;
		}
		console.log(radio_configs);
		
		for (i in radio_configs){ //generate HTML objects now
			rs = document.getElementById('radio_configs');
			rs.add(new Option(radio_configs[i]["freq"] + "M " + radio_configs[i]["bitrate"] + "kbps", radio_configs[i]["path"]));
		}
	} else {
		radio_configs_other_received(config_type, config_paths)
	}
}

function current_radio_config_received(current_config){
	console.log("current_radio_config_received");
	console.log(current_config);
	var c = radio_config_path_extract(current_config);
	lbl = document.getElementById('current_radio_config');
	if (c === null){
		lbl.innerHTML = "Current: None Loaded!";
	} else {
		lbl.innerHTML = "Current: " + c["freq"] + "M " + c["bitrate"] + "kbps";
	}
}

function current_radio_config_other_received(current_config){
	console.log("current_radio_config_other_received");
	txt = document.getElementById('current_radio_general_config')
	txt.innerHTML = current_config[0];
	txt = document.getElementById('current_radio_packet_config')
	txt.innerHTML = current_config[1];
	txt = document.getElementById('current_radio_preamble_config')
	txt.innerHTML = current_config[2];
}

function retrieve_username(){
	my_username = Cookies.get('username');
	if (my_username === undefined){
		my_username = generate_username();
		Cookies.set('username',my_username, {expires:COOKIE_EXP})
	} else {
		default_username = false;
	}
	document.getElementById("username_label").innerHTML = "Username: " + my_username;
	return my_username;
}

function retrieve_location_beacon_ms(){
	var location_beacon_ms = localStorage.getItem("location_beacon_ms");
	if (!location_beacon_ms){
		location_beacon_ms = 10000; //set to default
		localStorage.setItem("location_beacon_ms",location_beacon_ms); 
	}
	return location_beacon_ms;
}

function set_tab_buttons(disabled=false){
	document.getElementById("chat_button").disabled = disabled;
	document.getElementById("map_button").disabled = disabled;
	document.getElementById("settings_button").disabled = disabled;	
}

$(document).ready(function() {
	var ip = location.host;
	document.getElementById("instructions").innerHTML = "If you are in a captive portal browser the application will not work, please navigate to https://" + ip + " in chrome or firefox"
	
	set_tab_buttons(disabled=true);
	open_tab("splash");
});

handlers = {
	"newMessage" : other_user_message_cb,
	"location"   : other_user_location_cb,
	"waypoint"   : other_user_waypoint_cb,
	"get_radio_configs" : radio_configs_received,
	"get_current_radio_config" : current_radio_config_received,
	"get_current_radio_config_other" : current_radio_config_other_received
};

var messenger = new Worker("message_worker.js");
messenger.onmessage = (e) => {
	const handler = handlers[e.data[0]];
	if (handler === undefined){
		console.log("unknown header received from worker: " + e.data[0]);
		return;
	}
	handler(e.data[1],e.data[2]);
};

function user_accepted() {
	messenger.postMessage(["start",{'username':retrieve_username(),'location_beacon_ms':retrieve_location_beacon_ms()}]);
	
	set_tab_buttons(disabled=false);
	map = L.map('themap');

	var vectorTileStyling = {
		water: {
			fill: true,
			weight: 1,
			fillColor: '#06cccc',
			color: '#06cccc',
			fillOpacity: 0.2,
			opacity: 0.4,
		},
		admin: {
			weight: 1,
			fillColor: 'pink',
			color: 'pink',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		waterway: {
			weight: 1,
			fillColor: '#2375e0',
			color: '#2375e0',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		landcover: {
			fill: true,
			weight: 1,
			fillColor: '#53e033',
			color: '#53e033',
			fillOpacity: 0.2,
			opacity: 0.4,
		},
		landuse: {
			fill: true,
			weight: 1,
			fillColor: '#e5b404',
			color: '#e5b404',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		park: {
			fill: true,
			weight: 1,
			fillColor: '#84ea5b',
			color: '#84ea5b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		boundary: {
			weight: 1,
			fillColor: '#c545d3',
			color: '#c545d3',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		aeroway: {
			weight: 1,
			fillColor: '#51aeb5',
			color: '#51aeb5',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		transportation: {	// openmaptiles only
			weight: 0.5,
			fillColor: '#f2b648',
			color: '#f2b648',
			fillOpacity: 0.2,
			opacity: 0.4,
	// 					dashArray: [4, 4]
		},
		building: {
			fill: true,
			weight: 1,
			fillColor: '#2b2b2b',
			color: '#2b2b2b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		water_name: {
			weight: 1,
			fillColor: '#022c5b',
			color: '#022c5b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		transportation_name: {
			weight: 1,
			fillColor: '#bc6b38',
			color: '#bc6b38',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		place: {
			weight: 1,
			fillColor: '#f20e93',
			color: '#f20e93',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		housenumber: {
			weight: 1,
			fillColor: '#ef4c8b',
			color: '#ef4c8b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		poi: {
			weight: 1,
			fillColor: '#3bb50a',
			color: '#3bb50a',
			fillOpacity: 0.2,
			opacity: 0.4
		},


		// Do not symbolize some stuff for mapbox
		//country_label: [],
		//marine_label: [],
		//state_label: [],
		//place_label: [],
		//waterway_label: [],
		//poi_label: [],
		//road_label: [],
		//housenum_label: [],

		// Do not symbolize some stuff for openmaptiles
		country_name: {
			weight: 1
		},
		marine_name: {
			weight: 1
		},
		state_name: {
			weight: 100,
			fill: true,
			color: '#c0c0c0',
		},
		place_name: {
			weight: 1
		},
		waterway_name: {
			weight: 1
		},
		poi_name: {
			weight: 1
		},
		road_name: {
			weight: 1
		},
		housenum_name: {
			weight: 1
		},
		point: { color: "transparent" },
	};

	var openmaptilesVectorTileOptions = {
		rendererFactory: L.canvas.tile,
		//attribution: '<a href="https://openmaptiles.org/">&copy; OpenMapTiles</a>, <a href="http://www.openstreetmap.org/copyright">&copy; OpenStreetMap</a> contributors',
		vectorTileLayerStyles: vectorTileStyling,
		maxZoom: 14,
		minZoom: 14,
		//getFeatureId: function (e) {return}
	};

	var tile_grid = L.vectorGrid.protobuf("/tiles/{z}/{x}/{y}.pbf", openmaptilesVectorTileOptions).addTo(map);
	
	//map.on('click', function(e) {});
	
	setup_waypoints(map);
	setup_button_callbacks();
	get_current_radio_config();
	get_current_radio_config_other()
	
	///end of map setup///////////////////////////
	
	
	//init the radio button settings
	var load_on_start = Cookies.get('load_on_start');
	if (load_on_start === undefined){
		load_on_start = "load_chat";	
		Cookies.set('load_on_start',load_on_start, {expires:COOKIE_EXP})
	}
	document.getElementById(load_on_start).checked = true;
	if (load_on_start == "load_chat"){
		$('#chat_button').click();
		$('#messageText').focus();		
	}
	else {
		$('#map_button').click();
	}
	
	setup_notifications();
	
	setup_db(msg_db_cb, user_db_cb);
	setup_location_watch();
	
//////////////////////
    //console.log(Date.now())
    //socket = io.connect({query: "username=anonymous"});
    //socket.emit('time', {'timestamp': Date.now()});

	/*
	var startTime = new Date().getTime();
	map.on('movestart', function(evt) {
		startTime = new Date().getTime();
	});

	map.on('moveend', function(evt) {
		endTime = new Date().getTime();
		console.log('elapsed: ' + (endTime - startTime));
	});
	*/
	return false;
}