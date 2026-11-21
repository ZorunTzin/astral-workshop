EXTERN WorkshopMonsterCaptionHook:PROC
PUBLIC CaptionTestInvoke
PUBLIC CaptionTestResume
.code
; Model the native renderer's aligned mid-function stack and live RSI/RAX.
CaptionTestInvoke PROC FRAME
 push rsi
 .pushreg rsi
 .endprolog
 mov rsi,rcx
 mov rax,rdx
 jmp WorkshopMonsterCaptionHook
CaptionTestInvoke ENDP
CaptionTestResume PROC
 pop rsi
 ret
CaptionTestResume ENDP
END
