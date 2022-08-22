STATE:

  * test only collects logs right now (doesn't look at results)
  * Right now you can run test with (it only collec):

     $  PATH=$PWD/test:$PATH ./test/harness run --inputs-dir=inputs/ \
             --stubby=stubby.efi --sbat=sbat.csv test/tests.yaml
  * --boot-mode is efi or uefi_shell . ideally we integrate that into tests/
  * tests wont actually pass with upstream stubby, as bug
    https://github.com/puzzleos/stubby/issues/19

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
   * go-collect to grab 'inputs/' from system, or possibly download known good
     ones.
   * make - create stubby.efi

* Once per harness run:

   * check inputs
   * if encrypted, decrypt signing key to tmpdir/signing.key else copy to
     tmpdir/signing.key
   * sign the shim with signing key creating shim-signed.efi.

* Once per test run
kernel initramfs builtin-cmdline sbat
shim
cmdline
key
cert


   * create kernel.efi.nosign
     inputs: kernel initramfs builtin-cmdline sbat.csv
     output: kernel.efi.nosign
   * sign kernel.efi.nosign:
     inputs: kernel.efi.nosign
     outputs: kernel.efi
   * startup.nsh: no variables
   * launch.nsh:
     inputs: use-shim, cmdline
   * genesp:
     inputs:
   * write the startup.nsh and launch.nsh scripts.
     * launch.nsh is different if running shim.
   * boot
   * create esp, takes as input:
      
   * boot-vm takes as inputs:
        kernel-signed.efi
        --secure-boot=on / --secure-boot=off
        log-file
        esp

Notes:
 * jammy ovmf files with snakeoil are broken
   https://bugs.launchpad.net/ubuntu/+source/edk2/+bug/1986692
 * each run/<testdir> has a 'boot' script that can be run and will run the
   test interactively.  that's nice for getting to sehll and seeing things.
 * in the future it'd be nice if that could potentially re-generate the
   esp.img if you made changes.

 * there are 3 "modes" of boot.  "first loader" indicates either shim.efi
   (which then loads kernel.efi) or kernel.efi directly:
   * first loader specified in nvram entry (bcfg or efibootmgr)
   * first loader executed from startup.nsh script
   * first loader named bootx64.efi finds grubx64.efi .. no current way for
     cmdline arguments this way.  This is how a removble media would operate.
     We'd talked about potentially having a simplistic menu or something to
     read a file and pick one from a list.

 * bcfg can be used to write nvram entries
   https://superuser.com/questions/1035522/how-to-add-booting-parameters-to-the-kernel-with-bcfg-from-the-efi-shell

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
