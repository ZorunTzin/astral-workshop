; This is a mid-function continuation, not a normal function call.
; Preserve volatile GP/SIMD registers and flags before resuming relocated code.
EXTERN WorkshopMonsterCaptionAppend:PROC
EXTERN WorkshopMonsterCaptionResume:QWORD
PUBLIC WorkshopMonsterCaptionHook
.code
WorkshopMonsterCaptionHook PROC FRAME
    sub rsp, 0c0h
    .allocstack 0c0h
    .endprolog
    mov [rsp+080h], rax
    mov [rsp+088h], rcx
    mov [rsp+090h], rdx
    mov [rsp+098h], r8
    mov [rsp+0a0h], r9
    mov [rsp+0a8h], r10
    mov [rsp+0b0h], r11
    pushfq
    pop qword ptr [rsp+0b8h]
    movdqu [rsp+020h], xmm0
    movdqu [rsp+030h], xmm1
    movdqu [rsp+040h], xmm2
    movdqu [rsp+050h], xmm3
    movdqu [rsp+060h], xmm4
    movdqu [rsp+070h], xmm5
    mov rcx, rsi
    mov rdx, rax
    call WorkshopMonsterCaptionAppend
    movdqu xmm0, [rsp+020h]
    movdqu xmm1, [rsp+030h]
    movdqu xmm2, [rsp+040h]
    movdqu xmm3, [rsp+050h]
    movdqu xmm4, [rsp+060h]
    movdqu xmm5, [rsp+070h]
    mov rax, [rsp+080h]
    mov rcx, [rsp+088h]
    mov rdx, [rsp+090h]
    mov r8, [rsp+098h]
    mov r9, [rsp+0a0h]
    mov r10, [rsp+0a8h]
    mov r11, [rsp+0b0h]
    push qword ptr [rsp+0b8h]
    popfq
    lea rsp, [rsp+0c0h]
    jmp qword ptr [WorkshopMonsterCaptionResume]
WorkshopMonsterCaptionHook ENDP
END
