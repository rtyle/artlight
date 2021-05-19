const tau = 2 * Math.PI
const phi = (1 + Math.sqrt(5)) / 2
const inFirstTurn = theta => theta % tau
const toCartesian = (rho, theta) => [rho * Math.cos(theta), rho * Math.sin(theta)]
const ordinals = function*(end, begin = 0) {for (var i = begin; i < end; ++i) {yield i}}
const toCanvas = (x, y) => [x, -y]
const toPixels = mm => mm / 0.254
const ring = Array.from(ordinals(55))
	.map(i => 1024 - 1 - i)
	.map(i => [18 * Math.sqrt(1 + i), inFirstTurn(i * tau / phi)])
	.sort((a, b) => Math.sign(a[1] - b[1]))
	.map(polar => toCanvas(...toCartesian(...polar)))
const path = [...ring.map((v, i) => [0 == i ? 'M' : 'L', ...v].join(' ')), 'Z'].join(' ')
api('createShape', {
	shapeType: 'COPPERAREA',
	jsonCache: {
		layerid		: '1',
		clearanceWidth	: toPixels(0.3),
		toBoardOutline	: toPixels(0.3),
		fillStyle	: 'solid',
		thermal		: 'spoke',
		spoke_width	: 0,
		keepIsland	: 'none',
		pathStr		: path,
		locked		: 0,
	}
})
