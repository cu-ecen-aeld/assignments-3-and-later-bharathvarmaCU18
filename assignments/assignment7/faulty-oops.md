# Redirecting echo to faulty Driver  

echo “hello_world” > /dev/faulty  
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000  
Mem abort info:  
  ESR = 0x0000000096000045  
  EC = 0x25: DABT (current EL), IL = 32 bits  
  SET = 0, FnV = 0  
  EA = 0, S1PTW = 0  
  FSC = 0x05: level 1 translation fault  
Data abort info:  
  ISV = 0, ISS = 0x00000045  
  CM = 0, WnR = 1  
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041b64000  
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000  
Internal error: Oops: 0000000096000045 [#1] SMP  
Modules linked in: hello(O) faulty(O) scull(O)  
CPU: 0 PID: 152 Comm: sh Tainted: G           O       6.1.44 #1  
Hardware name: linux,dummy-virt (DT)  
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)  
pc : faulty_write+0x10/0x20 [faulty]  
lr : vfs_write+0xc8/0x390  
sp : ffffffc008df3d20  
x29: ffffffc008df3d80 x28: ffffff8001aa5cc0 x27: 0000000000000000  
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000  
x23: 0000000000000012 x22: 0000000000000012 x21: ffffffc008df3dc0  
x20: 0000005566fbc990 x19: ffffff8001c12e00 x18: 0000000000000000  
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000  
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000  
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000  
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000  
x5 : 0000000000000001 x4 : ffffffc000787000 x3 : ffffffc008df3dc0  
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000  
Call trace:  
 faulty_write+0x10/0x20 [faulty]  
 ksys_write+0x74/0x110  
 __arm64_sys_write+0x1c/0x30  
 invoke_syscall+0x54/0x130  
 el0_svc_common.constprop.0+0x44/0xf0  
 do_el0_svc+0x2c/0xc0  
 el0_svc+0x2c/0x90  
 el0t_64_sync_handler+0xf4/0x120  
 el0t_64_sync+0x18c/0x190  
Code: d2800001 d2800000 d503233f d50323bf (b900003f)  
---[ end trace 0000000000000000 ]---  


## Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000  
### This indicates we are trying to deference a NULL pointer somewhere in our faulty driver.  


## Analyzing call trace  

 faulty_write+0x10/0x20 [faulty]  
 ksys_write+0x74/0x110  
 __arm64_sys_write+0x1c/0x30  
 invoke_syscall+0x54/0x130  
 el0_svc_common.constprop.0+0x44/0xf0  
 do_el0_svc+0x2c/0xc0  
 el0_svc+0x2c/0x90  
 el0t_64_sync_handler+0xf4/0x120  
 el0t_64_sync+0x18c/0x190  


This the order of functions that were called bottom to top before encountering the oops message  

## Objdump of faulty.ko  


`
buildroot/output/host/bin/aarch64-linux-objdump -S buildroot/output/build/ldd-898d80fabf5fec212854da56e45a2f8f1f0d3403/misc-modules/faulty.ko  

buildroot/output/build/ldd-898d80fabf5fec212854da56e45a2f8f1f0d3403/misc-modules/faulty.ko:     file format elf64-littleaarch64  


Disassembly of section .text:  

0000000000000000 <faulty_write>:  
   0:	d2800001 	mov	x1, #0x0                   	// #0  
   4:	d2800000 	mov	x0, #0x0                   	// #0  
   8:	d503233f 	paciasp  
   c:	d50323bf 	autiasp  
  10:	b900003f 	str	wzr, [x1]  
  14:	d65f03c0 	ret  
  18:	d503201f 	nop  
  1c:	d503201f 	nop  
`

From the call trace, it is indicated that error happens at faulty_write_address + 16.   
The total size of the faulty_write function 32 bytes or 0x20 bytes.  
From the objdump we can see, we are trying to store wzr to address 0. we moved 0 to x1.  
##   10:	b900003f 	str	wzr, [x1]  
