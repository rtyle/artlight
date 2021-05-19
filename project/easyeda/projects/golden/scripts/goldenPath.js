const tau = 2 * Math.PI
const phi = (1 + Math.sqrt(5)) / 2
const inFirstTurn = theta => theta % tau
const toCartesian = (rho, theta) => [rho * Math.cos(theta), rho * Math.sin(theta)]
const toPolar = (x, y) => [Math.sqrt(x * x + y * y), inFirstTurn(tau + Math.atan2(y, x))]
const ordinals = function*(end, begin = 0) {for (var i = begin; i < end; ++i) {yield i}}
const seedCount = 1024
const seeds = function(seedCount, scale) {
	// grow seeds as in a sunflower head from the inside out
	return Array.from(ordinals(seedCount)).map(i => [scale * Math.sqrt(1 + i), inFirstTurn(i * tau / phi)])
}(seedCount + 144, 18)
const strip = function(seedCount, seeds) {
	// return one, roughly increasing, non-crossing, spiral strip through all the seeds
	const remaining = new Set(ordinals(seedCount))
	const spiral = []
	var i = seedCount - 1
	spiral.unshift(i)
	remaining.delete(i)
	while (remaining.size) {
		i = function() {
			// relative to i, spin the remaining seeds
			const a = seeds[i]
			const spin = seeds.map((b, j) => {if (!remaining.has(j)) {return undefined} else {
				const c = toCartesian(b[0], b[1] - a[1])
				return toPolar(c[0] - a[0], c[1])
			}})
			// find the smallest acceptable angle in the spin
			const acceptable = (tau * 128 / 1024)	// manually tuned
				+ spin[Array.from(remaining).sort((j, k) => Math.sign(spin[j][1] - spin[k][1]))[0]][1]
			// order the spin by nearest
			const nearest = Array.from(remaining).sort((j, k) => Math.sign(spin[j][0] - spin[k][0]))
			// return the nearest in spin with an acceptable angle
			var j = 0; for (;; ++j) {if (0 <= acceptable - spin[nearest[j]][1]) break}
			return nearest[j]
		}()
		spiral.unshift(i)
		remaining.delete(i)
	}
	return spiral
}(seedCount, seeds)
const toCanvas = (x, y) => [x, -y]
const addPolar = (a, b) => {
	const t = b[1] - a[1]
	const cos = Math.cos(t)
	const sin = Math.sin(t)
	return [Math.sqrt(a[0]**2 + b[0]**2 + 2 * a[0] * b[0] * cos), a[1] + Math.atan2(b[0] * sin, a[0] + b[0] * cos)]
}
const toDegree = r => r * 360 / tau
const toPixels = mm => mm / 0.254
const designatorOf = footprint => Object.keys(footprint.TEXT).reduce((v, t) => {const T = footprint.TEXT[t]; return 'P' == T.type ? T.text : v})
const numberIn = s => parseInt(s.match(/[0-9]+/)[0])
const netOf = {
	vcc:	step => '+5V',
	cko:	step => `D${step + 1}_5`,
	sdo:	step => `D${step + 1}_6`,
	sdi:	step => `D${0 < step ? step : 1}_${0 < step ? 6 : 1}`,
	cki:	step => `D${0 < step ? step : 1}_${0 < step ? 5 : 2}`,
	gnd:	step => 'GND',
}
api('setOriginXY', {x: 0, y: 0})
var json = api('getSource', {type: "json"})
var step = 0
Object.keys(json.FOOTPRINT)
	.filter(gId => 'package`SK9822 LED`' == json.FOOTPRINT[gId].head.c_para)
	.sort((gIdA, gIdB) => [gIdA, gIdB]
		.map(gId => numberIn(designatorOf(json.FOOTPRINT[gId])))
		.reduce((v, n) => undefined === v ? n : Math.sign(v - n))
	)
	.forEach(gId => {
		const part = designatorOf(json.FOOTPRINT[gId])
		const polar = seeds[strip[step]]
		const cartesian = toCanvas(...toCartesian(...polar))
                api('moveObjsTo', {objs: [{gId: gId}], x: cartesian[0], y: cartesian[1]})
		const rotate = inFirstTurn(tau * 7 / 4 - polar[1])
                api('rotate', {ids: [gId], degree: toDegree(rotate)})
                console.log(`move (#${step}, ${gId}, ${part})}) to (${cartesian[0]}, ${cartesian[1]}) and rotate ${toDegree(rotate)}`)
		const z = Object.keys(netOf).length
		var i = 0
		Object.keys(netOf).forEach(pad => {
			const p = toCanvas(...toCartesian(...addPolar(polar, [10, tau / 4 - rotate + tau * (3 * i + (i < z / 2 ? 3 : 6)) / (4 * z)])))
			api('createShape', {
				shapeType: 'VIA',
				jsonCache: {
					gId:		`_${part}_${pad}`,
					diameter:	toPixels(0.9),
					holeR:		toPixels(0.6 / 2),
					layerid:	11,
					x:		p[0],
					y:		p[1],
					net:		netOf[pad](step),
				},
			})
			++i
		})
                ++step
	})
