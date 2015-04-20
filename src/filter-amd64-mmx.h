         // Warning, number of coefficients is hardcoded.
          asm volatile(
		//"emms\n\t"
		"push %%rbp\n\t"
		"push %%rax\n\t"
                "push %%rdi\n\t"
		"push %%rcx\n\t"

                "mov %%rcx,%%rbp\n\t"
                "bigloop_mmx"FILTMMX_SKIP_ADD_STR":\n\t"

		"pxor %%mm1,%%mm1\n\t"
                "mov $32, %%rcx\n\t"

                "frup_mmx"FILTMMX_SKIP_ADD_STR":\n\t"

		/*
		PMADDWD mm1,mm2/m64           ; 0F F5 /r             [PENT,MMX] 

		dst[0-31]   := (dst[0-15] * src[0-15]) 
                               + (dst[16-31] * src[16-31]); 
		dst[32-63]  := (dst[32-47] * src[32-47]) 
	                               + (dst[48-63] * src[48-63]);

		*/
		"movq (%%rdi), %%mm0\n\t"
		"pmaddwd (%%rsi), %%mm0\n\t"
		"movq 8(%%rdi), %%mm2\n\t"
		"pmaddwd 8(%%rsi), %%mm2\n\t"

                "movq 16(%%rdi), %%mm3\n\t"
                "pmaddwd 16(%%rsi), %%mm3\n\t"
                "movq 24(%%rdi), %%mm4\n\t"
                "pmaddwd 24(%%rsi), %%mm4\n\t"


                "add $32, %%rdi\n\t"
		"add $32, %%rsi\n\t" 
		"paddd %%mm0, %%mm1\n\t"
		"paddd %%mm2, %%mm1\n\t"
		"paddd %%mm3, %%mm1\n\t"
		"paddd %%mm4, %%mm1\n\t"
		"dec %%rcx\n\t"
		"jnz frup_mmx"FILTMMX_SKIP_ADD_STR"\n\t"

                "movq %%mm1, (%%rax)\n\t"
                "sub $1024, %%rdi\n\t"
                "sub $1024, %%rsi\n\t"

                "add $4, %%rax\n\t"
                "add %%rdx, %%rdi\n\t"
                "dec %%rbp\n\t"
                "jnz bigloop_mmx"FILTMMX_SKIP_ADD_STR"\n\t"

		"pop %%rcx\n\t"
                "pop %%rdi\n\t"
		"pop %%rax\n\t"
		"emms\n\t"

		"convmmx"FILTMMX_SKIP_ADD_STR":\n\t"

		"mov (%%rax), %%rbp\n\t"
		//"sar $15, %%rbp\n\t"
		"sar $47, %%rbp\n\t"
		#ifndef FILTMMX_SKIP_ADD
		"add $32767, %%rbp\n\t"
		#endif
		"mov %%rbp, (%%rax)\n\t"

		"fild (%%rax)\n\t"
		"fstp (%%rax)\n\t"
		"add $4, %%rax\n\t"
		"dec %%rcx\n\t"
		"jnz convmmx"FILTMMX_SKIP_ADD_STR"\n\t"
		"pop %%rbp\n\t"
          :
          : "D" (in), "S" (ff->coeffs_i16), "a" (flout), "d" (2*ff->mrratio), "c" (bigcount)
          );
