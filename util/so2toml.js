//
// so2toml.js <z64.me>
//
// convert sharpocarina actor/object
// database xml's to z64scene toml's
//
// dependencies:
//  - xml2json (npm install xml2json)
//

// TODO handle object database xml's too
// TODO lots of cleanup

// show usage
if (process.argv.length < 3)
{
	console.log("arguments: %s so2toml ActorNames.xml [actors.toml]", process.argv0)
	console.log("e.g node so2toml bin/ootActorNames.xml toml/oot/actors.toml")
	return
}

// include fs module
const fs = require('fs')
const path = require('path')
const xml2json = require('xml2json')
//const tomlify = require('tomlify-j0.4') // testing

// program arguments
const _inFilename = path.resolve(process.argv[2])
const _outFilename = process.argv.length == 4 ? path.resolve(process.argv[3]) : undefined

// xml loader
require.extensions['.xml'] = function(module, filename) {
	// Function to recursively process JSON object
	function replaceProperty(obj, oldName, newName) {
		if (typeof obj === 'object' && obj !== null) {
			// If the object is an array, recursively process each item
			if (Array.isArray(obj)) {
				obj.forEach(item => replaceProperty(item, oldName, newName))
			} else {
				// If the object has a property named oldName, rename it to newName
				if (oldName in obj) {
					obj[newName] = obj[oldName]
					delete obj[oldName]
				}
				// Recursively process child properties
				for (const key in obj) {
					if (Object.prototype.hasOwnProperty.call(obj, key)) {
						replaceProperty(obj[key], oldName, newName)
					}
				}
			}
		}
		return obj
	}
	var _xml = fs.readFileSync(filename, 'utf8')
	
	module.exports = replaceProperty(JSON.parse(xml2json.toJson(_xml)), "$t", "tagInnerText")
}

// load the room
const _test = require(_inFilename)
//console.log(_test)
//console.log(JSON.stringify(_test.Table.Actor[0], null, 4))
//return;

var _result = []

for (const _actor of _test.Table.Actor)
{
	var _newActor = {
		Name: _actor.Name,
		Index: _actor.Key,
		Category: _actor.Category,
		Notes: _actor.Notes,
		Objects: _actor.Object,
	}
	var _variableMask = 0xffff
	
	// remove anything undefined
	for (const _key in _newActor)
		if (_newActor[_key] === undefined)
			delete _newActor[_key]
	
	// these are required
	if (_newActor.Name == undefined
		|| _newActor.Index == undefined
	)
	{
		console.log("actor missing name or index")
		continue
	}
	
	// unexpected formatting
	if (_newActor.Index.length != 4)
	{
		console.log(`actor '${_newActor.Name}' index unexpected formatting: ${_newActor.Index}`)
		continue
	}
	_newActor.Index = parseInt('0x' + _newActor.Index)
	
	// this should never happen, but squash them all down to one
	if (Array.isArray(_newActor.Notes))
	{
		console.log(`actor '${_newActor.Name}' multiple notes`)
		
		_newActor.Notes = _newActor.Notes.join('\n')
	}
	
	// this is acceptable
	if (_newActor.Objects == undefined)
	{
		//console.log(`actor ${_newActor.Name} missing object`)
		//continue
	}
	else if (_newActor.Objects.includes(","))
	{
		//console.log(`actor ${_newActor.Name} multiple objects: ${_newActor.Objects}`)
	}
	else if (_newActor.Objects.length != 4)
	{
		console.log(`actor ${_newActor.Name} objects unexpected formatting: ${_newActor.Objects}`)
		continue
	}
	
	// convert Objects to array of integers
	if (_newActor.Objects != undefined)
	{
		var _tmp = _newActor.Objects.split(",")
		
		_newActor.Objects = _tmp.map(_each => parseInt('0x' + _each))
	}
	
	// reference data:
	//Properties="FF00,00FF" PropertiesNames="Gold Skulltula Low Byte,Gold Skulltula High Byte" PropertiesTarget="Var,ZRot"
	if (_actor.Properties != undefined)
	{
		var _properties = _actor.Properties.split(",").map(_each => parseInt('0x' + _each))
		var _propertiesNames = _actor.PropertiesNames.split(",")
		var _propertiesTargets = []
		var _actorDropdownValueArrays = undefined
		var _actorDropdownNameArrays = undefined
		
		// split these by semicolon
		if (_actor.Dropdown != undefined && false) // turned off; unpredictable, and data sizes unexpected
		{
			_actorDropdownValueArrays = _actor.Dropdown.split(";")
			_actorDropdownNameArrays = _actor.DropdownNames.split(";")
			
			for (var _i = 0; _i < _actorDropdownValueArrays.length; ++_i)
				_actorDropdownValueArrays[_i] =
					_actorDropdownValueArrays[_i].split(",").map(_each => parseInt('0x' + _each))
			
			for (var _i = 0; _i < _actorDropdownNameArrays.length; ++_i)
				_actorDropdownNameArrays[_i] = _actorDropdownNameArrays[_i].split(",")
		}
		
		// if targets not specified, assume type 'Var' for each
		if (_actor.PropertiesTarget != undefined)
			_propertiesTargets = _actor.PropertiesTarget.split(",")
		else
			_propertiesTargets = Array(_properties.length).fill("Var")
		
		if (_properties.length != _propertiesNames.length
			|| _properties.length != _propertiesTargets.length
		)
		{
			console.log(`actor '${_newActor.Name}' properties unexpected formatting`)
		}
		else
		{
			_newActor.Property = []
			
			for (const [_index, _thisProperty] of _properties.entries())
			{
				var _thisName = _propertiesNames[_index]
				var _thisTarget = _propertiesTargets[_index]
				var _prop = {
					Mask: _thisProperty,
					Name: _thisName,
					Target: _thisTarget
				}
				
				// mark these bits as off-limits for any presets that will be generated later
				if (_thisTarget == "Var")
					_variableMask &= ~_thisProperty
				
				// interpret optional dropdown data
				if (_actorDropdownValueArrays != undefined)
				{
					var _thisValueArray = _actorDropdownValueArrays[_index]
					var _thisNameArray = _actorDropdownNameArrays[_index]
					var _dropDownArray = []
					
					for (const [_index, _thisValue] of _thisValueArray.entries())
					{
						var _thisName = _thisNameArray[_index]
						var _dropdown = [ parseInt('0x' + _thisValue), _thisName ]
						
						_dropdownArray.push(_dropdown)
					}
					
					_prop.Dropdown = _dropdownArray
				}
				
				_newActor.Property.push(_prop)
			}
		}
	}
	
	// convert variable array to presets
	if (_actor.Variable != undefined)
	{
		if (_newActor.Property == undefined)
			_newActor.Property = []
		
		if (!Array.isArray(_actor.Variable))
		{
			//console.log(`actor ${_newActor.Name} variable is not array (converting to one)`)
			
			_actor.Variable = [ _actor.Variable ]
		}
		
		if (!_variableMask && _actor.Variable.length)
		{
			console.log(`actor ${_newActor.Name} _variableMask == 0 (this shouldn't happen)`)
		}
		
		// determine how much to shift presets
		var _shiftEach = 0
		var _tmp = _variableMask
		while (_tmp && !(_tmp & 1))
		{
			_tmp >>= 1
			_shiftEach += 1
		}
		
		var _prop = {
			Mask: _variableMask,
			Name: "Presets",
			Target: "Var"
		}
		var _dropdownArray = []
		
		for (const _v of _actor.Variable)
		{
			var _dropdown = [ parseInt('0x' + _v.Var) >> _shiftEach, _v.tagInnerText ]
			
			_dropdownArray.push(_dropdown)
		}
		
		_prop.Dropdown = _dropdownArray
		
		_newActor.Property.push(_prop)
	}
	
	_result.push(_newActor)
}

//console.log(_result)
//console.log(JSON.stringify(_result, null, 4))
//console.log(JSON.stringify(_test.Table.Actor, null, 4))
//console.log(tomlify.toToml({_result}, {space: 2}))

// construct a toml
_toml = ""
_indentLevel = 0
function PrintIndent() { return "\n" + "\t".repeat(_indentLevel) }
function InsertAt(originalString, insertString, index) {
	if (index < 1) index += originalString.length
	return `${originalString.slice(0, index)}${insertString}${originalString.slice(index)}`;
}
for (const _actor of _result)
{
	_toml += PrintIndent() + "[[Actor]]"
	_indentLevel += 1
		if (_actor.Name) _toml += PrintIndent() + `Name = "${_actor.Name}"`
		_toml += PrintIndent() + `Index = ${_actor.Index}`
		if (_actor.Category != undefined) _toml += PrintIndent() + `Category = "${_actor.Category}"`
		if (_actor.Notes != undefined) {
			_toml += PrintIndent() + `Notes = "${_actor.Notes.replace(/\n/g, '\\n').replace(/"/g, '\\"')}"`
		}
		if (_actor.Objects) _toml += PrintIndent() + `Objects = ${JSON.stringify(_actor.Objects)}`
		if (_actor.Property)
		{
			//console.log(_actor.Property)
			for (const _prop of _actor.Property)
			{
				_toml += PrintIndent() + "[[Actor.Property]]"
				_indentLevel += 1
					_toml += PrintIndent() + `Mask = ${_prop.Mask}`
					_toml += PrintIndent() + `Name = "${_prop.Name}"`
					_toml += PrintIndent() + `Target = "${_prop.Target}"`
					if (_prop.Dropdown)
					{
						var _dropdown = JSON.stringify(_prop.Dropdown)
						_indentLevel += 1
						var _thisIndent = PrintIndent()
						_indentLevel -= 1
						
						_dropdown = InsertAt(_dropdown, _thisIndent, 1)
						_dropdown = _dropdown.replace(/\],\[/g, `],${_thisIndent}[`)
						_dropdown = InsertAt(_dropdown, PrintIndent(), -1)
						
						_toml += PrintIndent() + `Dropdown = ${_dropdown}`
					}
				_indentLevel -= 1
			}
		}
	_indentLevel -= 1
	_toml += PrintIndent()
}

// optionally write output file
if (_outFilename)
	fs.writeFileSync(_outFilename, _toml)
else
	console.log(_toml)
