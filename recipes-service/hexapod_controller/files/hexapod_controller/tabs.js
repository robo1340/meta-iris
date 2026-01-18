
const COOKIE_EXP=30;
//const LOAD_WAIT_MS=2000;
var state;
var user_input;


class Util {

	static contains_object(list, obj) {
		for (const ele of list){
			if (ele == obj) {return true;}
		}
		return false;
	}

	static safe_execute(func, ...args){
		try {
			return func(args);
		} catch { 
			return null;
		}
	}
	
	static path_basename(path){
		return path.split('\\').pop().split('/').pop();
	}
	
	static get_iso_timestamp(date) {
		var date = new Date();
		return date.toLocaleString('sv').replace(' ', 'T');
	}
	
	static get_cookie(name, default_return=undefined, parser=null){
		try {
			var val = Cookies.get(name);
			//console.log(name, val);
			if (val === undefined){
				Cookies.set(name,default_return, {expires:COOKIE_EXP});
				return default_return;
			} else if (parser !== null){
				return parser(val);
			} else {
				return val;
			}
		} catch { 
			return default_return;
		}
	}
	
	static randint(max) {
	  return Math.floor(Math.random() * max);
	}
	
	static randfloat(min, max){
		return (Math.random() * (max-min) + min);
	}
	
	static hex(i,pad=2){
		return i.toString(16).padStart(pad,'0').toUpperCase();
	}
	
	static user_string(user){
		return user.callsign + "(" + Util.hex(user.addr,4) + ")"
	}
	
	static float_equals(f1, f2, delta=0.01){
		//console.log("float_equals("+f1+","+f2+")");
		//console.log(Math.abs(f1-f2));
		//console.log(delta + " " + (Math.abs(f1-f2) < delta));
		return (Math.abs(f1-f2) < delta);
	}
	
	//calculate the distance between two markers
	static get_distance(from, to){
		//console.log('get_distance');
		if ((from === undefined) || (to === undefined)) {return -1;}
		//console.log(from);
		//console.log(to);
		if ((from.lat !== undefined) && (from.lng !== undefined)){
			from = from;
		} else if (from.getLatLng !== undefined){
			from = from.getLatLng();
		} else {
			console.log('ERROR: get_distance',from);
			return -1;
		}
		if ((to.lat !== undefined) && (to.lng !== undefined)){
			to = to;
		} else if (to.getLatLng !== undefined){
			to = to.getLatLng();
		} else{
			console.log('ERROR: get_distance',to);
			return -1;
		}
		//console.log(from);
		//console.log(to);

		var to_return = from.distanceTo(to);
		if (to_return > 10000){return -1;} //return invalid if greater than 100km
		//console.log('get_distance()->' + to_return);
		return to_return.toFixed(0);
	}
	
	static iso_to_time(iso_str){
		try {
			return iso_str.split('T')[1].split('.')[0];
		} catch {return "";}
	}

	static iso_to_date(iso_str) {
		try {
			return iso_str.split('T')[0];
		} catch {return "";}
	}
	
}


class State {
	constructor() {
		this.key_hold_time_ms 		= Util.get_cookie('key_hold_time_ms', 20, parseInt);
		this.expires 				= Util.get_cookie('expires', 100, parseInt);
		//this.my_callsign 			= Util.get_cookie('my_callsign', undefined);
		this.dst_addr				= Util.get_cookie('dst_addr', 0, parseInt);
		this.send_keypress_local 	= Util.get_cookie('send_keypress_local','false')=='true';
	}
}

class View {	
	static open_tab(target) {
		var i, tabcontent, tablinks;

		// Get all elements with class="tabcontent" and hide them
		tabcontent = document.getElementsByClassName("tabcontent");
		for (i=0; i<tabcontent.length; i++) {
			tabcontent[i].style.display = "none";
		}

		// Get all elements with class="tablinks" and remove the class "active"
		tablinks = document.getElementsByClassName("tablinks");
		for (i=0; i<tablinks.length; i++) {
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
}


class UserInput {
	
	static key_remap = {
		'arrowup' : 'up',
		'arrowdown' : 'down',
		'arrowleft' : 'left',
		'arrowright' : 'right',
		' '			: 'space'
	}
	
	static sliders = ['tx','ty','tz','pitch','roll','yaw','speed','stride_length','step_height','corner_rotation']
	
	constructor(){
		this.key_press_timers = {};
		
		this.key_pressed 	= this.key_pressed.bind(this);
		this.key_released 	= this.key_released.bind(this);
		
		document.addEventListener('keydown', this.key_pressed);
		document.addEventListener('keyup'  , this.key_released);
		
		document.getElementById("send_keypress_local").onclick = this.send_keypress_local;
		document.getElementById("expires").addEventListener("input", this.expires);
		document.getElementById("key_hold_time_ms").addEventListener("input", this.key_hold_time_ms);
		
		document.getElementById("dst_addr").addEventListener('change', this.dst_addr); 
		
		for (const slider of UserInput.sliders) {
			document.getElementById(slider).oninput = function() {UserInput.slider_moved(slider);}
		}
		
		const intervalId = setInterval(this.send_sliders, 1000);
		
		//document.getElementById("send_button").onclick = UserInput.send_msg;
		//document.getElementById("destination_address").addEventListener('change', this.destination_address); 
		//document.getElementById("randomize_locations").onclick = this.randomize_locations;
		//document.getElementById("location_period_s").addEventListener("input", this.location_period_s);
	}
	
	send_sliders() {
		for (const id of UserInput.sliders){
			let value = document.getElementById(id).value;
			messenger.postMessage(['ui_control', UserInput.ui_control_msg(id, value)]);
		}
	}
	
	static slider_moved(id){
		let value = document.getElementById(id).value;
		messenger.postMessage(['ui_control', UserInput.ui_control_msg(id, value)]);
	}
	
	dst_addr(){
		console.log('dst_addr')
		let addr = parseInt(this.value);
		if (Number.isNaN(addr)==false){
			state.dst_addr = addr;
		}
	}
	
	key_hold_time_ms(e){
		let x = parseInt(e.target.value);
		if (Number.isNaN(x)){return;}
		state.key_hold_time_ms = x;
		Cookies.set('key_hold_time_ms',state.key_hold_time_ms, {expires:COOKIE_EXP});
	}
	
	expires(e){
		let x = parseInt(e.target.value);
		if (Number.isNaN(x)){return;}
		state.expires = x;
		Cookies.set('expires',state.expires, {expires:COOKIE_EXP});
	}
	
	static ui_control_msg(id,value){
		return {
			'type' : 'ui_control',
			'dst'	: 0,
			'src'	: (state.send_keypress_local==true) ? 0 : undefined,
			'hops_start' : 1,
			'id' : id,
			'value' : value/100,
			'send_keypress_local' : state.send_keypress_local
		};
	}
	
	static key_press_msg(e, pressed=false, held=false, released=false){
		let key = e.key.toLowerCase();
		if (UserInput.key_remap[key] !== undefined){
			key = UserInput.key_remap[key];
		}
		return {
			'type' : 'key_press',
			'dst'	: 0,
			'src'	: (state.send_keypress_local==true) ? 0 : undefined,
			'expires' : state.expires,
			'hops_start' : 1,
			'pressed' : pressed,
			'held' : held,
			'released' : released,
			'key' : key,
			'send_keypress_local' : state.send_keypress_local
		}
	}
	
	key_pressed(e){
		if (this.key_press_timers[e.key] != undefined){return;}
		//console.log('key_pressed', e);
		messenger.postMessage(['key_press', UserInput.key_press_msg(e, true, false, false)]);

		this.key_press_timers[e.key] = setInterval(function() {
			UserInput.key_held(e);
		}, state.key_hold_time_ms);
	}
	
	key_released(e){
		//console.log('key_released', e);
		messenger.postMessage(['key_press', UserInput.key_press_msg(e, false, false, true)]);

		if (this.key_press_timers[e.key] != undefined){
			clearInterval(this.key_press_timers[e.key]);
			delete this.key_press_timers[e.key];
		}
	}
	
	static key_held(e){
		//console.log('key_held', e);
		messenger.postMessage(['key_press', UserInput.key_press_msg(e, false, true, false)]);
	}
	
	send_keypress_local(){
		state.send_keypress_local = !state.send_keypress_local;
		let b = document.getElementById("send_keypress_local");
		b.checked = state.send_keypress_local;
		Cookies.set('send_keypress_local',state.send_keypress_local, {expires:COOKIE_EXP});
	}
	
}

//$(document).read()
window.onload = function() {
	var ip = location.host;
	View.open_tab('settings_button')
	main();
	
};


//handlers for when message is received from the backend client process
class Handlers {
	constructor(initial_handlers) { //setup initial handlers, when data bases are opened call this.init()
		this.handlers = initial_handlers;
		//console.log("Constructor of:", this.constructor.name);
	}
	
	static new_addr(topic, data, timestamp){
		console.log(topic, data, timestamp);
	}
	
	handle(topic, data, timestamp){
		//console.log('Handlers.handle from backend',topic,data,timestamp);
		const h = this.handlers[topic];
		if (h !== undefined){
			h(topic, data, timestamp);
		}
	}
	
}


var handler = new Handlers({
	"hexapod" : Handlers.new_addr,
	"rover"	  : Handlers.new_addr
});


var messenger = new Worker("message_worker.js");
messenger.onmessage = function(e) {
	handler.handle(e.data[0],e.data[1],e.data[2]);
};


function main() {
	
	state = new State();
	user_input = new UserInput();
	console.log('main()',state);

	document.getElementById("send_keypress_local").checked = state.send_keypress_local;
	document.getElementById('key_hold_time_ms').value = state.key_hold_time_ms;
	document.getElementById('expires').value = state.expires;
	document.getElementById('dst_addr').value = state.dst_addr
	
	return false; //why does this return false?
}
