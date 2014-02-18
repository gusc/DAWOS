; Program flow:
; 1. we are loaded at 0x7C00
; 2. init and store drive ID
; 3. do a check on MBR parition table for GPT protective record
; 4. do a check for LBA capabilities (extended read/write)
; 5. read GPT header
; 6. do a check on GPT signature
; 7. do a check on GPT partition count (should be more than or equal to 1)
; 8. read first 4 entries from GPT partition array
; 9. do a check on each partition entry for GUID of BBP
; 10. store BBP partition's start LBA
; 10. calculate and store BBP partition's lenght
; 11. relocate rest of the MBR (just the part that loads data from BBP) to 0x0500
; 12. read BBP from the disk and store in memory starting at 0x7C00
; 13. jump to BBP (0x7C00)
;
; memory map at the start:
; 0x0500 - 0x7BFF - free
; 0x7C00 - 0x7DFF - MBR loaded by BIOS
; 0x7E00 - 0x0007FFFF - free
;
; memory map before the jump to BBP:
; 0x1000 - Disk address packet (DAP) location (2x qword)
; 0x1080 - Local data (disk ID, bbp size, etc.) location (2x dword)
; 0x1100 - Read buffer location (512 bytes usually)
; 0x7C00 - Stack pointer
; 0x7C00 - 0x7DFF - MBR loaded by BIOS
; 0x7E00 - 0x80000 - Bootloader code loaded by MBR bootstrap (we have atleast 480KiB or 960 sectors here)

; Code location constants
%define ORG_LOC         0x7C00                      ; Initial MBR position in memory (where BIOS loads it)
%define BOOT_LOC        0x7E00                      ; Location of BBP bootcode
%define BOOT_SEG        0x07E0                      ; BBP bootcode segment

%define DAP_LOC         0x1000                      ; DAP location
%define DATA_LOC        0x1080                      ; Location of our global data structure in memory
%define BUFF_LOC        0x1100                      ; Location of read buffer in memory
%define TRANSF_SECTORS  0x0001                      ; Transfer 64 sectors at a time
%define TRANSF_BYTES    0x0200                      ; Transfer 32kb at a time (512b * 64)
%define TRANSF_MAX      0x03C0                      ; 960 sectors max

; Local data structure
struc tDATA                            
    .drive_id        resb    1                      ; drive ID
    ._pad            resb    3                      ; dummy padding
    .bbp_size        resd    1                      ; BBP sector count
endstruc
%define DATA(x)  DATA_LOC + tDATA. %+ x             ; Helper macro to clean up the code a bit

; MBR Partition entry structure
struc tMBRPart
    .status         resb    1                       ; Partition status (0x80 if bootable, 0x00 otherwise)
    .chs_start      resb    3                       ; Start of partition as a CHS value (we're not going to use this)
    .type           resb    1                       ; Partition type (we're looking for 0xEE)
    .chs_end        resb    3                       ; End of partition as a CHS value
    .lba_start      resd    1                       ; Start of partition as a LBA value
    .lba_length     resd    1                       ; Size of partition - number of sectors
endstruc
%define MBR_LOC         0x7DBE                      ; Partition table start
%define MBR(x)  MBR_LOC + tMBRPart. %+ x            ; Helper macro to clean up the code a bit

; DAP structure for extended read
struc tDAP
    .size           resb    1                       ; Packet size
    .reserved       resb    1                       ; Reserved - should be 0
    .lba_count      resw    1                       ; Number of sectors to transfer
    .dest_buff      resw    1                       ; Desination buffer where to transfer data
    .dest_segm      resw    1                       ; Destination segment
    .lba_start_l    resd    1                       ; Low dword of start LBA
    .lba_start_h    resd    1                       ; High dword of start LBA
endstruc
%define DAP(x)    DAP_LOC + tDAP. %+ x              ; Helper macro to clean up the code a bit

; GPT Header
struc tGPTHead
    .signature      resd    2                       ; EFI PART signature
    .version        resd    1                       ; Version number
    .header_size    resd    1                       ; Size of a header (should be 92)
    .header_crc32   resd    1                       ; CRC32 of first 3 entries
    .reserved       resd    1                       ; Should be 0
    .lba_curr       resd    2                       ; Current address of GPT header
    .lba_backup     resd    2                       ; Address of backup GPT header
    .lba_first      resd    2                       ; First usable LBA
    .lba_last       resd    2                       ; Last usable LBA
    .disk_guid      resd    4                       ; Disk GUID
    .part_arr_lba   resd    2                       ; LBA adress of partition array
    .part_count     resd    1                       ; Number of entries in partition array
    .part_size      resd    1                       ; Size of partition entry
    .part_arr_crc32 resd    1                       ; CRC32 of partition array
    ; There should be a definition of 420 reserved bytes, bet we don't need them
endstruc
%define GPT(x)    BUFF_LOC + tGPTHead. %+ x         ; Helper macro to clean up the code a bit

; GPT Partition
struc tGPTPart
    .part_guid      resd    4                       ; Partition type GUID
    .unique_guid    resd    4                       ; Unique GUID of partition
    .lba_start_l    resd    1                       ; Start LBA low dword
    .lba_start_h    resd    1                       ; Start LBA high dword
    .lba_end_l      resd    1                       ; End LBA low dword
    .lba_end_h      resd    1                       ; End LBA high dword
    .attributes     resd    2                       ; Partition attributes
    .name           resw    36                      ; UTF-16 partition name
endstruc
%define PART(x)    BUFF_LOC + tGPTPart. %+ x        ; Helper macro to clean up the code a bit 

; Remember in NASM it's:
; instruction destination, source

[org ORG_LOC]                                       ; set origin at 0x7C00
[bits 16]                                           ; tell NASM that we are working in 16bit environment
    jmp 0x0000:start

start:                                              ; Start MBR procedures
    xor eax, eax                                    ; clear eax (is this some kind of asm developers way of seting value to 0? why can't we use
                                                    ; mov ax, word 0x0000 instead? is this faster or cleaner?)
    mov ss, ax                                      ; set stack segment to zero (is it, i'm dumb in assembly?)
    mov sp, ORG_LOC                                 ; set stack pointer to the begining of MBR location in memory
    mov es, ax                                      ; zero-out extra segment
    mov ds, ax                                      ; zero-out data segment
.setup_data:
    mov [DATA(drive_id)], dl                        ; store drive ID into memory
.setup_dap:
    mov [DAP(size)], byte 0x10                      ; set packet size
    mov [DAP(reserved)], byte 0x00                  ; set reserved byte
    mov [DAP(lba_count)], word 0x0001               ; set number of blocks to transfer
    mov [DAP(dest_buff)], dword BUFF_LOC            ; set read buffer position in memory
    mov [DAP(dest_segm)], dword 0x0                 ; set read buffer segment
    mov [DAP(lba_start_l)], dword 0x0               ; clear start LBA low dword
    mov [DAP(lba_start_h)], dword 0x0               ; clear start LBA high dword

test_mem:                                           ; Get available lower memory ammount
    int 0x12                                        ; call memory interrupt
    jc $                                            ; hang if interrupt call failed
    test ax, ax                                     ; check if memory is more than 0 (weird instruction - got to wrap my head around this)
    jz $                                            ; hang if ax is 0 (right?)
    sub ax, 0x001F                                  ; 0x7C00 / 1024 = 0x001F
                                                    ; we need to know if we have a region between 0x7C00 and 0xA0000 which is at least 512KiB
    cmp ax, 0x0200                                  ; test weather it's enough
    jl $                                            ; hang if not enough memory    
    
test_mbr_part:                                      ; Test weather we are a protective MBR, we certanly don't want to be a hybrid MBR
    cmp [MBR(type)], byte 0xEE                      ; check weather we have GPT partitioning (0xEE - GPT protective partition)
    jne $                                           ; hang if not a protective partition

test_ext:                                           ; Check if drive supports extended read
    mov ah, 0x41                                    ; command - check for extensions
    mov bx, 0x55AA                                  ; some magic test value
    mov dl, [DATA(drive_id)]                        ; set drive ID
    int 0x13                                        ; call disk interrupt
    jc $                                            ; hang if extensions are not supported
    cmp bx, 0xAA55                                  ; extra test for return value from extension test
    jne $                                           ; hang if the return value is not the one we expected

reset_disk:                                         ; Reset current disk
    mov ah, 0x00                                    ; command - reset disk
    mov dl, [DATA(drive_id)]                        ; set drive ID
    int 0x13                                        ; call disk interrupt
    jc $                                            ; hang if reset failed

read_gpt:                                           ; Read GPT header
    mov [DAP(lba_count)], word 0x0001               ; set number of blocks to transfer
    mov [DAP(lba_start_l)], dword 0x00000001        ; set start block address (low dword, 2nd block)
    mov ah, 0x42                                    ; command - extended read
    mov dl, [DATA(drive_id)]                        ; set drive ID
    mov si, DAP_LOC                                 ; set DAP location
    int 0x13                                        ; call disk interrupt
    jc $                                            ; hang if disk read failed
    
test_gpt:                                           ; Parse GPT signature (should be "EFI PART" - look up ASCII yourself)
    ;cmp dword [GPT(signature)], 0x20494645         ; should be "EFI " (or " IFE" in little-endian order)
    ;jne $                                          ; hang - not even close
    ;cmp dword [GPT(signature) + 4], 0x54524150     ; should be "PART" (or "TRAP", pun intended?)
    ;jne $                                          ; hang - not quite there
    
read_part_arr:                                      ; Read Partition array
    mov [DAP(lba_start_l)], dword 0x00000002        ; set start block address (low dword, 3d block)
    mov ah, 0x42                                    ; command - extended read
    mov dl, [DATA(drive_id)]                        ; set drive ID
    mov si, DAP_LOC                                 ; set DAP location
    int 0x13                                        ; call disk interrupt
    jc $                                            ; hang if disk read failed
    mov word cx, 0x0004                             ; we only read first 4 partitions

test_part:                                          ; Read GPT partition entries to find if it's BBP (BIOS Boot Partition)
                                                    ; we only accept first partition to be BIOS bootable
    mov ax, word 0x0004                             ; set AX to 4 (we need it to calculate partition number)
    sub ax, cx                                      ; subtract current step from AX (thus partition number is 4-4=0, 4-3=1, etc)
    mov dx, word 0x0080                             ; set DX to 128 bytes (partition entry size)
    mul dx                                          ; multiply our result in AX to get the offset
    mov bx, ax                                      ; store offset in BX (I think it's base offset register, is it?)
                                                    ; and we test it, and it's uggly (GUID and little-endianness I mean)
    cmp dword [PART(part_guid) + bx], 0x21686148
    jne test_next                                   ; stop, you're no BBP
    cmp dword [PART(part_guid) + bx + 4], 0x6E6F6449
    jne test_next                                   ; nice try
    cmp dword [PART(part_guid) + bx + 8], 0x65654E74
    jne test_next                                   ; yeah, almost got me
    cmp dword [PART(part_guid) + bx + 12], 0x49464564
    jne test_next                                   ; sooo close
    jmp prepare_copy                                ; found it!

test_next:
    dec cx                                          ; one step back means one step forward in partition entry array
    cmp cx, word 0x0000                             ; check if we're not at the end of our 4 entry array
    jg test_part                                    ; try our luck on the next one
    jmp $                                           ; hang - no BBPs found

prepare_copy:                                       ; Prepare our BBP copy
    mov [DAP(dest_buff)], dword BOOT_LOC            ; set read buffer position in memory
    mov [DAP(lba_count)], word TRANSF_SECTORS       ; set the number of sectors to copy per iteration iteration
.test_start_lba:
    cmp dword [PART(lba_start_h) + bx], 0x0         ; test if the start LBA is on the near side of disk
    jne $                                           ; hang if BBP is out of bounds
.test_end_lba:
    cmp dword [PART(lba_end_h) + bx], 0x0           ; test if the end LBA is on the near side of disk
    jne $                                           ; hang if BBP is out of bounds
.cont_prep:
    mov eax, dword [PART(lba_end_l) + bx]           ; store low dword of end LBA
    sub eax, dword [PART(lba_start_l) + bx]         ; subtract low dword of start LBA
    mov [DATA(bbp_size)], eax                       ; store sector count in memory
    mov eax, dword [PART(lba_start_l) + bx]         ; get the low dword of start LBA
    mov [DAP(lba_start_l)], eax                     ; store it into DAP package
    mov eax, dword [PART(lba_start_h) + bx]         ; get the high dword of start LBA
    mov [DAP(lba_start_h)], eax                     ; store it into DAP package

copy_start:                                         ; Read bootable partition
    xor eax, eax                                    ; clear eax
    xor ebx, ebx                                    ; clear ebx
    xor ecx, ecx                                    ; clear ecx
    mov sp, 0x7C00                                  ; reset stack pointer to the begining of the new boot loader location
    mov eax, dword [DATA(bbp_size)]                 ; copy boot partition sector count
    cmp eax, TRANSF_MAX                             ; test if not bigger than max sectors
    jle .prep_copy_loop                             ; continiue if less or equal
    mov eax, TRANSF_MAX                             ; limit to max sectors
.prep_copy_loop:
    mov bx, word TRANSF_SECTORS                     ; copy the number of bytes to copy per iteration
    div bx                                          ; calculate iteration count
    mov cx, ax                                      ; set iteration count
.copy_block:
    mov ah, 0x42                                    ; command - extended read
    mov dl, [DATA(drive_id)]                        ; set drive ID
    mov si, DAP_LOC                                 ; set DAP location
    int    0x13                                     ; call disk interrupt
    jc $                                            ; halt on error
    dec cx                                          ; decrement cx for the next iteration
    cmp cx, 0x0                                     ; test if not the final iteration
    je .end                                         ; last iteration - pass control over to BBP code
    
    mov ax, [DAP(dest_segm)]                        ; get the segment index from DAP
    shr ax, 12                                      ; shift right by 12 bits
    mov [DAP(dest_segm)], ax                        ; save it back to use in the next block for increment

    mov eax, [DAP(dest_buff)]                       ; get current buffer position
    add eax, dword TRANSF_BYTES                     ; move forward by copied byte ammount (this also increments the segment part)
    mov [DAP(dest_buff)], eax                       ; set next buffer position

    mov ax, [DAP(dest_segm)]                        ; get the segment index from DAP again
    shl ax, 12                                      ; shift left by 12 bits
    mov [DAP(dest_segm)], ax                        ; save it back to use proper segmented address in disk read target

    mov eax, [DAP(lba_start_l)]                     ; get current LBA position
    add eax, dword TRANSF_SECTORS                   ; move forward by copied sector ammount
    mov [DAP(lba_start_l)], eax                     ; set next LBA position
    jmp .copy_block                                 ; next pass
.end:
    jmp 0x0000:BOOT_LOC                             ; pass control over to BBP code
copy_end:

padding:                                            ; Zerofill up to 440 bytes
    times 0x01B8 - ($ - $$) db 0

; This is needed so we don't overwrite partition table with our assembly code
part_table:
    times 0x0200 - 2 - ($ - $$) db 0

magic:                                              ; Add "magic number"
    dw 0xAA55                                       ; Magic number - boot sector signature
