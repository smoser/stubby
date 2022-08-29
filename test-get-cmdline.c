#include "kcmdline.h"
#include "linux_efilib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	EFI_STATUS expstatus;
	char *expected;
	BOOLEAN secure;
	char *builtin;
	char *runtime;
} TestData;


TestData tests[] = {
	// all good secure
	{EFI_SUCCESS, "root=atomix console=ttyS0 more", true,
		"root=atomix STUBBY_RT_CLI1 more", "console=ttyS0"},
	// all good secure marker at beginning
	{EFI_SUCCESS, "root=atomix console=ttyS0 more", true,
		"STUBBY_RT_CLI1 root=atomix more", "console=ttyS0"},
	// all good secure marker at end
	{EFI_SUCCESS, "root=atomix console=ttyS0 more", true,
		"root=atomix more STUBBY_RT_CLI1", "console=ttyS0"},
	// all good insecure
	{EFI_SUCCESS, "root=atomix console=ttyS0 more", false,
		"root=atomix STUBBY_RT_CLI1 more", "console=ttyS0"},
	// no marker in secureboot input
	{EFI_INVALID_PARAMETER, "root=atomix console=ttyS0", true,
		"root=atomix", "console=ttyS0"},
	// no marker in insecure - just append???
	{EFI_SUCCESS, "root=atomix console=ttyS0", false,
		"root=atomix", "console=ttyS0"},
	// namespace for marker found twice in builtin secure
	{EFI_INVALID_PARAMETER, "root=atomix console=ttyS0", true,
		"root=atomix STUBBY_RT debug STUBBY_RT_CLI1 ", "console=ttyS0"},
	// namespace for marker found twice in builtin insecure
	{EFI_INVALID_PARAMETER, "root=atomix console=ttyS0", false,
		"root=atomix STUBBY_RT debug STUBBY_RT_CLI1 ", "console=ttyS0"},
	// namespace appears in runtime
	{EFI_INVALID_PARAMETER, "root=atomix console=ttyS0", false,
		"root=atomix debug STUBBY_RT_CLI1", "console=ttyS0 STUBBY_RT"},
};


BOOLEAN do_get_cmdline(TestData td) {
	EFI_STATUS status;
	CHAR16 *errmsg;
	CHAR8 *found;
	UINTN found_len;
	CHAR16 buf[64];

	status = get_cmdline(
		td.secure, 
		(CHAR8*) td.builtin, strlen(td.builtin),
		(CHAR8*)td.runtime, strlen(td.runtime),
		&found, &found_len,
		&errmsg);

	StatusToString(buf, status);
	if (errmsg) {
		Print(L"errmsg: %ls", errmsg);
		free(errmsg);
	} else {
		Print(L"that was emtpy, got: %ls\n", buf);
	}

	return (status == td.expstatus);
}

int main()
{
	int num = sizeof(tests) / sizeof(tests[0]);
	int passes = 0, fails = 0;

	for (int i=0; i<num; i++) {
		if (do_get_cmdline(tests[i])) {
			passes++;
		} else {
			fails++;
		}
	}

	if (fails != 0) {
		return 1;
	}

	return 0;
}
