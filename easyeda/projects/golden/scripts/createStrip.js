api('setOriginXY', {x: 0, y: 0})

const uuid = 'b3e16c16cb7549c1a5adff5a8e69295f'

for (var i = 0; i < 1024; ++i) {
	api('createShape', {
		shapeType:	'schlib',
		shortUrl:	uuid,
		from:		'system',
		x:		i * 200,
		y:		0,
	})
}
