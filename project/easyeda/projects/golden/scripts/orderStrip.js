const uuid = 'b3e16c16cb7549c1a5adff5a8e69295f'

// may not be reflected in the itemOrder given to the created objects.
// the name given to a NET between pads of different objects
// will reflect the annotation of the lower ordered object.
// we would like the position of our objects
// to implicitly make NET connections between them and
// be able to predict their names.
// to this end,
// reposition the created objects next to each other
// in the order that they were given.

var json = api('getSource', {type:'json'})

var i = 0
json.itemOrder.filter(gId => {
	return uuid == json.schlib[gId].head.uuid
})
.forEach(gId => {
	api('moveObjsTo', {objs: [{gId: gId}], x: i * 100, y: 0})
	++i
})

// annotate them in the same order
// Edit: Annotate...
//	Method
//		X Re-annotate all
//	Direction
//		X Rows
