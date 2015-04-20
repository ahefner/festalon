        uint32 bigcount = max / ff->mrratio;

         // Warning, number of coefficients is hardcoded.
          asm volatile(
		"femms\n\t"
                "pushl %%ebp\n\t"
                "pushl %%edi\n\t"

                "movl %%ecx,%%ebp\n\t"
                "bigloopnow:\n\t"
		
		"pxor %%mm4, %%mm4\n\t"

                "movl $128, %%ecx\n\t"

                "frupnow:\n\t"

		"movq (%%edi), %%mm0\n\t"
		"movq (%%eax), %%mm1\n\t"
		"pfmul %%mm1, %%mm0\n\t"
		"movq 8(%%edi), %%mm2\n\t"
		"movq 8(%%eax), %%mm3\n\t"
		"pfmul %%mm3, %%mm2\n\t"
                "addl $16, %%edi\n\t"
                "addl $16, %%eax\n\t"
		"pfadd %%mm2, %%mm0\n\t"
		"pfadd %%mm0, %%mm4\n\t"
		"decl %%ecx\n\t"
                "jnz frupnow\n\t"

		"movq %%mm4, %%mm1\n\t"
		"movq %%mm4, %%mm2\n\t"

		"punpckldq %%mm4, %%mm1\n\t"
		"punpckhdq %%mm1, %%mm2\n\t"

		"pfadd %%mm2, %%mm4\n\t"

		"movd %%mm4, (%%esi)\n\t"
		"subl $2048, %%edi\n\t"
		"subl $2048, %%eax\n\t"

                "addl $4, %%esi\n\t"
                "addl %%edx, %%edi\n\t"
                "decl %%ebp\n\t"
                "jnz bigloopnow\n\t"

                "popl %%edi\n\t"
                "popl %%ebp\n\t"
		"femms\n\t"
          :
          : "D" (in), "a" (ff->coeffs), "S" (flout), "d" (4*ff->mrratio), "c" (bigcount)
          );
