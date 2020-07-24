for y in range(16):
	for n in range(2):
		for x in range(16):
			print('X,X, ', end = '')

			if x < 15:
				print('0, ', end = '')

		if n == 1:
			print()

		print()

	for x in range(16):
		print('X,X, ', end = '')
		if x < 15: 
			print('0, ', end = '')
	
	for x in range(16):
		print('X,X, ', end = '')
		if x < 15: 
			print('0, ', end = '')
	
	print()
	
	if y < 15:
		for x in range(16):
			print('0,0, ', end = '')
			if x < 15:
				print('0, ', end = '')

	print()
	print()