#ifndef __STUBBY_CMDLINE_H
#define __STUBBY_CMDLINE_H

#include <efi.h>

EFI_STATUS check_cmdline(CONST CHAR8 *cmdline, UINTN cmdline_len, CHAR16 **errmsg);

EFI_STATUS get_cmdline(
	BOOLEAN secure,
	CONST CHAR8 *builtin, UINTN builtin_len,
	CONST CHAR8 *runtime, UINTN runtime_len,
	CHAR8 **cmdline, UINTN *cmdline_len,
	CHAR16 **errmsg);

EFI_STATUS get_cmdline_with_print(
	BOOLEAN secure,
	CONST CHAR8 *builtin, UINTN builtin_len,
	CONST CHAR8 *runtime, UINTN runtime_len,
	CHAR8 **cmdline, UINTN *cmdline_len);

#endif
