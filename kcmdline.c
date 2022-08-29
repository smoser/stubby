#include <efi.h>
#ifdef LINUX_TEST
#include "linux_efilib.h"
#else
#include <efilib.h>
#endif

#include "kcmdline.h"
#include "stra.h"


// If a provided command line has more tokens (words) than MAX_TOKENS
// then an error will be returned.
#define MAX_TOKENS 128

// These are the tokens that are allowed to be passed on EFI cmdline.
static const CHAR8 allowed[][32] = {
	"^console=",
	"^root=oci:",
	"root=atomix",
	"ro",
	"quiet",
	"verbose",
	"crashkernel=256M",
};

BOOLEAN is_allowed(const CHAR8 *input) {
	int len = 0;
	int input_len = strlena(input);
	const CHAR8 * token;
	int num_allowed = sizeof(allowed) / sizeof(allowed[0]);
	for (int i=0; i<num_allowed; i++) {
		token = allowed[i];
		len = strlena(token);
		if (token[0] == '^') {
			len = len - 1;
			token = &token[1];
		} else if (input_len != len) {
			continue;
		}
		if (strncmpa(token, input, len) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

// check cmdline to make sure it contains only allowed words.
// return EFI_SUCCESS on safe, EFI_SECURITY_VIOLATION on unsafe;
EFI_STATUS check_cmdline(CONST CHAR8 *cmdline, UINTN cmdline_len, CHAR16 **errmsg) {
	CHAR8 c = '\0';
	CHAR8 *buf = NULL;
	CHAR16 *errbuf = NULL;
	int errbuf_len = (cmdline_len + 80);
	CHAR8 *tokens[MAX_TOKENS];
	*errmsg = NULL;
	EFI_STATUS status = EFI_SUCCESS;
	int i;
	int start = -1;
	int num_toks = 0;
	buf = AllocatePool(cmdline_len + 1);
	if (buf == NULL) {
		goto out;
		status = EFI_OUT_OF_RESOURCES;
	}

	errbuf = AllocatePool(errbuf_len * sizeof(CHAR16));
	if (errbuf == NULL) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}
	errbuf[0] = '\0';

	CopyMem(buf, cmdline, cmdline_len);
	buf[cmdline_len] = '\0';

	// allow for cmdline_len to have included the terminating null
	if (cmdline_len > 1 && buf[cmdline_len-1] == '\0') {
		cmdline_len--;
	}

	// walk buf, populating tokens
	for (i = 0; i < cmdline_len; i++) {
		c = buf[i];
		if (c < 0x20 || c > 0x7e) {
			UnicodeSPrint(errbuf, errbuf_len,
				L"Bad character 0x%02hhx in position %d: %a.\n", c, i, cmdline);
			status = EFI_SECURITY_VIOLATION;
			goto out;
		}
		if (i >= MAX_TOKENS) {
			UnicodeSPrint(errbuf, errbuf_len, L"Too many tokens in cmdline.\n");
			status = EFI_SECURITY_VIOLATION;
			goto out;
		}

		if (c == ' ') {
			// end of a token
			buf[i] = '\0';
			if (start >= 0) {
				tokens[num_toks] = &buf[start];
				start = -1;
				num_toks++;
			}
		} else {
			if (start < 0) {
				start = i;
			}
		}
	}
	if (start >= 0) {
		tokens[num_toks] = &buf[start];
		num_toks++;
	}

	for (i=0; i < num_toks; i++) {
		if (!is_allowed(tokens[i])) {
			UnicodeSPrint(errbuf, errbuf_len, L"token not allowed: %a\n", tokens[i]);
			status = EFI_SECURITY_VIOLATION;
		}
	}

out:



	if (buf != NULL) {
		FreePool(buf);
	}
	if (errbuf != NULL && errbuf[0] == '\0') {
		FreePool(errbuf);
		errbuf = NULL;
	}
	*errmsg = errbuf;
	return status;
}

// produce the combined command line from builtin and runtime portions.
// string lengths builtin_len and runtime_len should not include the terminating null
// (as returned by strlen).
EFI_STATUS get_cmdline(
		BOOLEAN secure,
		CONST CHAR8 *builtin, UINTN builtin_len,
		CONST CHAR8 *runtime, UINTN runtime_len,
		CHAR8 **cmdline, UINTN *cmdline_len,
		CHAR16 **errmsg) {

	const CHAR8 *marker = (CHAR8 *)"STUBBY_RT_CLI1";
	const CHAR8 *namespace = (CHAR8 *)"STUBBY_RT";
	UINTN marker_len = strlena(marker);
	CHAR8 *p;
	CHAR8 *part1, *part2;
	UINTN part1_len, part2_len;
	EFI_STATUS status = EFI_SECURITY_VIOLATION;

	CHAR16 *errbuf = NULL;
	int errbuf_buflen = 255;
	errbuf = AllocatePool(errbuf_buflen * sizeof(CHAR16));
	if (errbuf == NULL) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	part1 = AllocatePool(builtin_len + 1);
	if (part1 == NULL) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	part2 = AllocatePool(builtin_len + 1);
	if (part2 == NULL) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	*part1 = '\0';
	*part2 = '\0';
	*errbuf = '\0';
	part1_len = 0;
	part2_len = 0;

	if (builtin_len != 0) {
		p = strstra(builtin, marker);
		if (p == NULL) {
			// there was no marker in builtin
			if (runtime_len != 0) {
				status = EFI_INVALID_PARAMETER;
				UnicodeSPrint(errbuf, errbuf_buflen,
						L"runtime arguments cannot be given to non-empty builtin without marker\n");
				goto out;
			}
		} else {
			// builtin has a marker, check that only one.
			if (strstra(p + marker_len, marker) != NULL) {
				status = EFI_INVALID_PARAMETER;
				UnicodeSPrint(errbuf, errbuf_buflen,
						L"%a XXXX appears more than once in builtin cmdline\n", marker);
				goto out;
			}

			part1_len = (p - builtin);
			part2_len = builtin_len - marker_len - part1_len;

			// Print(L"p == builtin? %d\n", p == builtin);
			// if (p != builtin) {
			// 	Print(L"*(p-1): '%c'\n", *(p-1));
			// }
			// Print(L"part2_len is zero? %d\n", part2_len == 0);
			// Print(L"*(p + marker_len + 1) = '%c'\n", *(p + marker_len));

			if (!((p == builtin || *(p-1) == ' ') &&
					(part2_len == 0 || *(p + marker_len) == ' '))) {
				status = EFI_INVALID_PARAMETER;
				UnicodeSPrint(errbuf, errbuf_buflen, L"%a is not a full token\n", marker);
			}

			CopyMem(part1, builtin, part1_len);
			*(part1 + part1_len) = '\0';
			CopyMem(part2, p + marker_len, part2_len);
			*(part2 + (part2_len)) = '\0';
		}
	}

	// namespace appeared in the builtin
	if (strstra(part1, namespace) != NULL || strstra(part2, namespace) != NULL) {
		status = EFI_INVALID_PARAMETER;
		UnicodeSPrint(errbuf, errbuf_buflen, L"%a appears in builtin cmdline\n", namespace);
		goto out;
	}

	// namespace appears in runtime
	if (strstra(runtime, namespace) != NULL) {
		status = EFI_INVALID_PARAMETER;
		UnicodeSPrint(errbuf, errbuf_buflen, L"%a appears in runtime cmdline\n", namespace);
		goto out;
	}

	// Print(L"part1 [%d] = '%a'\npart2 [%d] = '%a'\n", part1_len, part1, part2_len, part2);

	// At this point, part1 and part2 are set so we can just concat part1, runtime, part2

out:
	if (errbuf != NULL && errbuf[0] == '\0') {
		FreePool(errbuf);
		errbuf = NULL;
	}

	if (part1 != NULL)
		FreePool(part1);

	if (part2 != NULL)
		FreePool(part2);

	*errmsg = errbuf;
	return status;
}

EFI_STATUS get_cmdline_with_print(
		BOOLEAN secure,
		CONST CHAR8 *builtin, UINTN builtin_len,
		CONST CHAR8 *runtime, UINTN runtime_len,
		CHAR8 **cmdline, UINTN *cmdline_len) {

	CHAR16 *errmsg = NULL;
	EFI_STATUS err;

	err = get_cmdline(secure,
		builtin, builtin_len, runtime, runtime_len,
		cmdline, cmdline_len, &errmsg);

	if (!EFI_ERROR(err)) {
		goto out;
		return err;
	}

	if (errmsg == NULL) {
		Print(L"%r\n");
	} else {
		Print(L"%r: %ls\n", errmsg);
	}

	if (err == EFI_SECURITY_VIOLATION) {
		if (secure) {
			Print(L"Custom kernel command line rejected\n");
		} else {
			Print(L"Custom kernel would be rejected in secure mode\n");
		}
	}

out:
	if (errmsg != NULL) {
		FreePool(errmsg);
	}

	return err;
}
