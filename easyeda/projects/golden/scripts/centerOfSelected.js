console.log(api('getSelectedIds')
	.split(',')
	.reduce((v, i) => {
		const s = api('getShape', {id: i})
		const x =  s.head.x
		const y = -s.head.y
		return [
			[Math.min(v[0][0], x), Math.min(v[0][1], y)],
			[Math.max(v[1][0], x), Math.max(v[1][1], y)],
		]
	}, [
		[Number.MAX_SAFE_INTEGER, Number.MAX_SAFE_INTEGER],
		[Number.MIN_SAFE_INTEGER, Number.MIN_SAFE_INTEGER],
	])
	.reduce((v, i) => [v[0] + i[0], v[1] + i[1]], [0, 0])
	.map(i => i / 2)
	.map(i => i * .254)	// to mm
)
