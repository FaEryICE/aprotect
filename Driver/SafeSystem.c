#include "SafeSystem.h"

///////////////////////////////////////////////////////////////////////////////
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("Driver Unload Called\n"));
}

///////////////////////////////////////////////////////////////////////////////
VOID IsKernelBooting(IN PVOID Context)
{
	NTSTATUS	status;
	PUCHAR		KiFastCallEntry;
	ULONG		EProcess;
	int			i = 0;
	ULONG		ImageBase;

	if (PsGetProcessCount() <= 2)
		bKernelBooting = TRUE;
	else 
		goto _InitThread;  //ϵͳû�иո�������������

	while (1)
	{
		//ϵͳ�ո�����
		if (bKernelBooting)
		{
			//�ȴ���ʼ���ɹ�
			//��ʼ���ɹ���Ϊ����������
			//1��ע����ʼ�����ˣ����Է��ʡ�д��
			//2���ļ�ϵͳ��ʼ�����ˣ����Է����ļ���д���ļ�
			//3���û��ռ���̻����Ѿ���ʼ������      <== �����������ж���bug�������жϲ��ǱȽ�׼ȷ��

			if (PsGetProcessCount() >= 3)
			{
				break;
			}
		}

		WaitMicroSecond(88);
	}

	/* ϵͳ�ո�������ִ�е��������˵�������ɨ�衣�Ϳ�ʼ���
	   ��������ɨ�裬�Ϳ����������
	   ԭ��д��KeBugCkeckֵ��ring3������ʱ��ɾ�������ֵ�����������ɹ�������
	   ����´�������ʱ����KeBugCkeck���ڣ���˵���������ɹ�����ֱ�ӷ��أ������κβ���
	   ������������ֵ��˵���������ɹ���ֱ�ӷ��أ������κ�hook */
	if (IsRegKeyInSystem(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\KeBugCheck"))
	{	
		/* �������������ܵ���PsTerminateSystemThread�����߳�
		   Ӧ�����߳��Լ�����*/
		return;
	}

	//д��һ��ֵ���ڽ�������֮��ɾ�������������ɹ���
	//���ִ�е����˵���ȶ����в�������
	KeBugCheckCreateValueKey(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\KeBugCheck");

_InitThread:
	_asm
	{
		pushad;
		mov ecx, 0x176;
		rdmsr;
		mov KiFastCallEntry, eax;
		popad;
	}

	/* ��ֹ�ظ�����
	  ��ע�����ע�Ὺ������������U�̾ͻ�ڶ��μ��� */
	if (*KiFastCallEntry == 0xe9)
	{
		KdPrint(("Terminate System Thread"));

		/* �������������ܵ���PsTerminateSystemThread�����߳�
		   Ӧ�����߳��Լ����� */
		return;
	}

	if (ReLoadNtos(PDriverObject, RetAddress) == STATUS_SUCCESS)
	{
		//����������صı������������������ɨ��
		PsSetLoadImageNotifyRoutine(ImageNotify);

		//demo����ȷ���ɨ������
		if (bKernelBooting)
		{
			DepthServicesRegistry = (PSERVICESREGISTRY)ExAllocatePool(NonPagedPool,sizeof(SERVICESREGISTRY)*1024);
			if (DepthServicesRegistry)
			{
				memset(DepthServicesRegistry,0,sizeof(SERVICESREGISTRY)*1024);
				QueryServicesRegistry(DepthServicesRegistry);
			}
		}
	}

	bKernelBooting = FALSE;
}

///////////////////////////////////////////////////////////////////////////////
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING theRegistryPath)
{
	ULONG	ulSize;
	ULONG	ulKeServiceDescriptorTable;
	int		i = 0;
	HANDLE	HThreadHandle;
	HANDLE	ThreadHandle;

	DriverObject->DriverUnload = DriverUnload;
	PDriverObject = DriverObject;
	RetAddress = *(DWORD*)((DWORD)&DriverObject - 4);
	ulMyDriverBase = DriverObject->DriverStart;
	ulMyDriverSize = DriverObject->DriverSize;

	DebugOn = TRUE;  //������ʽ��Ϣ

	KdPrint(("//***************************************//\r\n"
	       	 "// A-Protect Anti-Rootkit Kernel Module  //\r\n"
			 "// Kernel Module Version LE 2012-0.4.3   //\r\n"
		     "// website:http://www.3600safe.com       //\r\n"
	         "//***************************************//\r\n"));

	// ��ȡ��ǰ�����������̵� EPROCESS
	SystemEProcess = PsGetCurrentProcess();

	// ��ȡϵͳ�汾
	WinVersion = GetWindowsVersion();
	if (WinVersion)
		KdPrint(("Init Windows version Success\r\n"));

	DepthServicesRegistry = NULL;
	
	//����һ��ϵͳ�߳�������
	if (PsCreateSystemThread(
			&HThreadHandle,
			0,
			NULL,
			NULL,
			NULL,
			IsKernelBooting,
			NULL) == STATUS_SUCCESS)
	{
		ZwClose(HThreadHandle);
	}

	return STATUS_SUCCESS;
}