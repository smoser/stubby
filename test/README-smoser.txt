Test things:

test.yaml looks like this:

    "01-sb-allowed":
      sb: true
      builtin: ""
      cli: "root=atomix console=ttyS0"
      assert:
        - booted-cmdline-cli


 * maybe need to chagen that to a list rather than a dict.
   the value in a dict was that you had a guaranted uniq name.
   the value of list is that you can have order without having
   to do the "01-" business.

   probably should just make the harness check all the test names
   and fail if there are some dups.


* Test setup:

   * install needed packages - ovmf, qemu-system
   * update apt and apt-get download shim-signed.
   * get-krd - creates kernel and initrd
   * make - create stubby.efi

* Once per harness run:

   * check inputs
   * if encrypted, decrypt signing key to tmpdir/signing.key else copy to
     tmpdir/signing.key
   * sign the shim with signing key creating shim-signed.efi.

* Once per test run
   * create kernel.efi.nosign
   * sign kernel.efi to kernel-signed.efi 
   * write the startup.nsh and launch.nsh scripts.
     * launch.nsh is different if running shim.
   * boot

Notes:
 * startup.nsh 
   * is selected for execution by the bootloader (special well known name)
   * startup.nsh runs 'launch.nsh' - this is because a .nsh script will
     effectively run witih "set -e" unless it is called by another nsh
     script.  this is crazy town but true.  So instead of just having
     startup.nsh launch our efi directly, we have it launch launch.nsh

     after executing launch.nsh, startup.nsh will shutdown.

 * launch.nsh will either execute:
      shim.efi kernel.efi args here
      or
      kernel.efi args here.
