//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//
// snd_mixa.s
// x86 assembly-language sound code
//

#include "../qasm.h"

#if	id386

	.extern C(snd_linear_count)
	.extern C(snd_p)
	.extern C(snd_out)

	.text

//----------------------------------------------------------------------
// Transfer of stereo buffer to 16-bit DMA buffer code
//----------------------------------------------------------------------

.globl C(S_WriteLinearBlastStereo16)
C(S_WriteLinearBlastStereo16):
	pushl	%edi
	pushl	%ebx

//	int		i;
//	int		val;
	movl	C(snd_linear_count),%ecx
	movl	C(snd_p),%ebx
	movl	C(snd_out),%edi

//	for (i=0 ; i<snd_linear_count ; i+=2)
//	{
LWLBLoopTop:

//		val = snd_p[i]>>8;
//		if (val > 0x7fff)
//			snd_out[i] = 0x7fff;
//		else if (val < (short)0x8000)
//			snd_out[i] = (short)0x8000;
//		else
//			snd_out[i] = val;
	movl	-8(%ebx,%ecx,4),%eax
	sarl	$8,%eax
	cmpl	$0x7FFF,%eax
	jg		LClampHigh
	cmpl	$0xFFFF8000,%eax
	jnl		LClampDone
	movl	$0xFFFF8000,%eax
	jmp		LClampDone
LClampHigh:
	movl	$0x7FFF,%eax
LClampDone:

//		val = snd_p[i+1]>>8;
//		if (val > 0x7fff)
//			snd_out[i+1] = 0x7fff;
//		else if (val < (short)0x8000)
//			snd_out[i+1] = (short)0x8000;
//		else
//			snd_out[i+1] = val;
	movl	-4(%ebx,%ecx,4),%edx
	sarl	$8,%edx
	cmpl	$0x7FFF,%edx
	jg		LClampHigh2
	cmpl	$0xFFFF8000,%edx
	jnl		LClampDone2
	movl	$0xFFFF8000,%edx
	jmp		LClampDone2
LClampHigh2:
	movl	$0x7FFF,%edx
LClampDone2:
	shll	$16,%edx
	andl	$0xFFFF,%eax
	orl		%eax,%edx
	movl	%edx,-4(%edi,%ecx,2)

//	}
	subl	$2,%ecx
	jnz		LWLBLoopTop

//	snd_p += snd_linear_count;

	popl	%ebx
	popl	%edi

	ret


#endif	// id386

