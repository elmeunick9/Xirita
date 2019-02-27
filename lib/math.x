[SUM a b] ASSIGN a , ADD b

[FIB 1] ASSIGN 1
[FIB 2] ASSIGN 1
[FIB n]
a	ASSIGN n , SUB 1 , FIB a
b	ASSIGN n , SUB 2 , FIB b
	SUM a b
	
