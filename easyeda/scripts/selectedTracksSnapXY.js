// an EasyEDA script to nudge points in selected tracks to the PCB snap grid

var json = api('getSource', {type: "json"})

var snapSize = json.canvas.snapSize
var origin = {x: json.canvas.originX, y: json.canvas.originY}

function snap(x) {
        return Math.round(x / snapSize) * snapSize
}

var ids = api('getSelectedIds')
if (ids.length) {
	ids.split(',').forEach(id => {
		if (json.TRACK.hasOwnProperty(id)) {
			var i = 0
			json.TRACK[id].pointArr.forEach(point => {
				['x', 'y'].forEach(ordinate => {
					var original = point[ordinate]
					var snapped = snap(original - origin[ordinate]) + origin[ordinate]
					if (original != snapped) {
						console.log(`TRACK[${id}].pointArr[${i}].${ordinate} = ${original} != ${snapped}`)
						point[ordinate] = snapped
					}
				})
				++i
			})
		}
	})
}

api('applySource', {source: json, createNew: false})
