import os
import sys
import json

## @brief safely lookup a key in a dictionary
## if the key does not exist, return a default value
## @param d the dictionary to lookup a value from
## @param key the key to lookup
## @param default the default value to return if key does not exist in d
## @param set_default the key can't be found and set_default is true, set the value of d[key] to default
def safe_lookup(d, key, default={}, set_default=True):
	if (type(d) is not dict):
		return default
	if not key in d:
		if (set_default is True):
			d[key] = default
		return default
	else:
		return d[key]

def safe_delete(d, key):
	if (type(d) is not dict):
		return False
	if key in d:
		d.pop(key)
		return True
	return False
		
## @brief read in the contents of a json file and write a default value to the file on read failure
## @return returns the contents of the json file on success, returns the default value on failure
def safe_read_json(path, silent=False, default={}):
	try:
		with open(path, 'r') as f:
			return json.load(f)
	except BaseException as ex:
		if (not silent):
			log.warning('could not open and decode json file \'%s\'' % (path,))
			log.warning(ex)
		return default

def safe_loads_json(to_load, silent=False):
	try:
		return json.loads(to_load)
	except BaseException as ex:
		if (not silent):
			log.warning('could not open and decode json file \'%s\'' % (path,))
			log.warning(ex)
		return None	

def safe_write_json(path, to_write):
	try:
		with open(path, 'w') as f:
			json.dump(to_write, f, indent=2)
	except BaseException as ex:
		log.warning('error while writing to json file at path \'%s\'' % (path,))
		log.warning(ex)
		
def safe_write_file(path, to_write):
	try:
		with open(path, 'w') as f:
			f.write(to_write)
	except BaseException as ex:
		log.warning('error while writing to file at path \'%s\'' % (path,))
		log.warning(ex)
		return False
	return True

def safe_read_file(path):
	try:
		with open(path, 'r') as f:
			return f.read()
	except BaseException as ex:
		log.warning('error while reading at path \'%s\'' % (path,))
		log.warning(ex)
		return None