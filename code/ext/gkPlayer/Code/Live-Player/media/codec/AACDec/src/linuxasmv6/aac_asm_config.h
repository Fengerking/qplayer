
.macro RM_EXPORT name, export=1
     .if HIDDEN_SYMBOL==1
        .hidden \name
     .endif
    .if \export
        .global \name
	.endif	
.endm

