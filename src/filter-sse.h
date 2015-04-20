        uint32 bigcount = max / ff->mrratio;

         // Warning, number of coefficients is hardcoded.
          asm volatile(
                "pushl %%ebp\n\t"
                "pushl %%edi\n\t"

                "movl %%ecx,%%ebp\n\t"
		#ifdef TMP_FSSE3
		"bigloop2:\n\t"
		#else
                "bigloop:\n\t"
		#endif
                "xorps %%xmm1,%%xmm1\n\t"
                "movl $64, %%ecx\n\t"

		#ifdef TMP_FSSE3
		"frup2:\n\t"
		#else
                "frup:\n\t"
		#endif
                "movaps (%%edi), %%xmm0\n\t"
                "mulps (%%esi), %%xmm0\n\t"
                "movaps 16(%%edi), %%xmm2\n\t"
                "mulps 16(%%esi), %%xmm2\n\t"
                "addl $32, %%edi\n\t"
                "addl $32, %%esi\n\t"
                "addps %%xmm0,%%xmm1\n\t"
                "addps %%xmm2,%%xmm1\n\t"
                "decl %%ecx\n\t"
		#ifdef TMP_FSSE3
                "jnz frup2\n\t"
		#else
		"jnz frup\n\t"
		#endif

		#ifdef TMP_FSSE3
                "haddps  %%xmm1, %%xmm1\n\t"
                "haddps  %%xmm1, %%xmm1\n\t"
		#else
                "movaps %%xmm1, %%xmm2\n\t"
                "shufps $0xb1, %%xmm2, %%xmm1\n\t"
                "addps %%xmm2, %%xmm1\n\t"
                "movaps %%xmm1, %%xmm2\n\t"
                "shufps $0x0a, %%xmm1, %%xmm1\n\t"
                "addps %%xmm2, %%xmm1\n\t"
		#endif
                "movss %%xmm1, (%%eax)\n\t"
                "subl $2048, %%edi\n\t"
                "subl $2048, %%esi\n\t"

                "addl $4, %%eax\n\t"
                "addl %%edx, %%edi\n\t"
                "decl %%ebp\n\t"
		#ifdef TMP_FSSE3
		"jnz bigloop2\n\t"
		#else
                "jnz bigloop\n\t"
		#endif

                "popl %%edi\n\t"
                "popl %%ebp\n\t"
          :
          : "D" (in), "S" (ff->coeffs), "a" (flout), "d" (4*ff->mrratio), "c" (bigcount)
          );
