[EQUALS x]  EQ x
[IF a EQ b]
t	ASSIGN a , EQUALS b
t	THEN:
		BLOCK
		RETURN
t	ELSE:
		RETURN
[LOOP a]
i	ASSIGN 0
b	ASSIGN i , EQ a , NOT
b	WHILE:
		BLOCK
i		ADD 1
b		ASSIGN i , EQ a , NOT
	RETURN
	
[MATH] IMPORT "lib/math.x"