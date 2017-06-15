#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <winstring.h>
#include <string.h>
#include <cstring>
#include <msclr/marshal_cppstd.h>
#include <vcclr.h>
#include <vector>
#include <sstream>
#include <array>
#include <iomanip>
#include <codecvt>
//#include <list>
#using <mscorlib.dll>

#pragma once

namespace SP_FM {

	using namespace System;
	using namespace System::Text;
	using namespace System::IO;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;
	using namespace std;
	typedef char* PSTR, *LPSTR;
#pragma pack(push,1)
	typedef struct _MBRptRec {
		bool isMain;	// or extended
		bool isBoot;	
		int sysCode;
		UINT BegAddr;	// Otn Addr
		UINT Size;		// of logic disk in MB
		UINT AbsSect;
		UCHAR* serNum;
		char* Name;  
		_MBRptRec* nextPTRec;
	} MBRptRec;
	typedef struct _MBR {
		WCHAR* PhysDrive;
		MBRptRec* ptRec;
	} MBR;	
	typedef struct _GPT_HEADER {
		DWORD64 Smech;	// 8b EFI_PART
		DWORD Version;		// 4b Version
		DWORD Header_size;  //4b header size
		DWORD Sum;  //4b header control summ
		DWORD smth;
		DWORD64 sygn; //1-st header addr
		DWORD64 secHeadAddr; //2d header address
		DWORD64 ldBegAddr; // after PT, logic disks info
		DWORD64 ldEndAddr; // before altern PT  
		GUID Dev_GUID;
		BYTE PT_Addr[8]; // начало PT
		DWORD amount_PT; // 4b кол-во записей PT
		DWORD size_PT;  // 4b - размер 1 записи PT
		DWORD Sum2;	//4b   //92 b
		BYTE other[420];
	} GPT_HPT;
	typedef struct _GPT_PT{
		GUID GUID_PT;
		GUID ID;
		DWORD64 AddrBeg;
		DWORD64 AddrEnd;
		DWORD64 Attr;
		CHAR Name[72];
	} GPT_PT;
	typedef struct _GPT {
		WCHAR* PhysDrive;
		GPT_HPT hpt = { 0,0,0,0,0,{ 0 },0,0,0,0 };
		GPT_PT tables[128] = { 0 };
	} GPT;
	typedef struct _DiskInfo {
		char Name[10]; // logic disk name
		WCHAR* PhysDrive; // hard drive
		char id[50]; // for guid
		DWORD ser;   // for serial number
		bool isSupported = true;
	} DiskInfo;
	typedef struct _Disks {
		DiskInfo disks[50];
	} LogDisks;
	typedef struct _BOOT
	{
		WORD sectSize; // amount of bytes in sector 11
		BYTE clustSize; //cluster size in sectors 13
		WORD reservSize; // reserv region size 14
		BYTE kFat;	// number f fat tables 16
		WORD maxRootRecCount;	// FAT32 = 0; FAT12/16 - max amount of root records 17
		DWORD fatSize; // for 2 model 36
		DWORD ldSize; // size of logical disk for 2 model 32 
		DWORD64 sizeServ; // size of service region (before fat)
		DWORD64 RootSize; // root size in sectors
		DWORD rootFirstClust; // first cluster of root, 2 model 44
		DWORD64 RootAddr; // first sector of root
	} BOOT;
	typedef struct _Descriptor {
		UCHAR Name[8];
		UCHAR  Ext[3];
		BYTE Attr;
		BYTE Other[8];
		WORD BegClust;
		BYTE Other2[4];
		WORD EndClust;   // [begClust(2b) endClust(2b)]
		DWORD Size;      // in bytes, 0 for directory
	}Descriptor;
	typedef struct _Catalog {
		Descriptor *descs = new Descriptor[16];
	}Catalog;
	typedef struct _FileInfo {
		int ind; // index of output element
		char *cname;
		WCHAR *wname;
		int type; // 1 - file, 0 - dir;
		int cluster; 
		Int64 size;
		_FileInfo *nextRec;
	} FileInfo;
	typedef struct _NumbersChain {
		DWORD cl;
		_NumbersChain *next;
	};
#pragma pack(pop)
	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		MyForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}
		LogDisks *disks = new LogDisks(); 
		int diskAmount = 0; 
		WORD SectSize = 512;
		MBR** mbrs = new MBR*[20];
		int mbrsN = 0;
		int gptsN = 0;
		GPT** gpts = new GPT*[20];
		BOOT *curBoot = new BOOT();
		Catalog *curCatPart = new Catalog();
		FileInfo *curDir = new FileInfo();
		DWORD *FAT;
		WCHAR* curHddName = new WCHAR[20];
		bool isMBR;
		DWORD64 curAbsFirSec;
		int curFS;
		char* curPath = new char[1024];
		DWORD rootCurClust;
	private: System::Windows::Forms::Button^  Refresh;
	public:
	private: System::Windows::Forms::DataGridView^  structs;
	private: System::Windows::Forms::Label^  hddCur;
	private: System::Windows::Forms::DataVisualization::Charting::Chart^  gist;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Disk;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Part;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Type;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Size;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Column1;
	private: System::Windows::Forms::Button^  btRoot;
	private: System::Windows::Forms::Button^  btShowEmpty;
	private: System::Windows::Forms::Label^  lbFS;


	private: System::Windows::Forms::Button^  Details;
	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::ListBox^  files;
	private: System::Windows::Forms::ComboBox^  ldList;
	private: System::Windows::Forms::TextBox^  path;
	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			System::Windows::Forms::DataVisualization::Charting::ChartArea^  chartArea3 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Series^  series3 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Title^  title3 = (gcnew System::Windows::Forms::DataVisualization::Charting::Title());
			this->files = (gcnew System::Windows::Forms::ListBox());
			this->ldList = (gcnew System::Windows::Forms::ComboBox());
			this->path = (gcnew System::Windows::Forms::TextBox());
			this->Refresh = (gcnew System::Windows::Forms::Button());
			this->structs = (gcnew System::Windows::Forms::DataGridView());
			this->Disk = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Part = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Type = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Size = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column1 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Details = (gcnew System::Windows::Forms::Button());
			this->hddCur = (gcnew System::Windows::Forms::Label());
			this->gist = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->btRoot = (gcnew System::Windows::Forms::Button());
			this->btShowEmpty = (gcnew System::Windows::Forms::Button());
			this->lbFS = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->structs))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->gist))->BeginInit();
			this->SuspendLayout();
			// 
			// files
			// 
			this->files->FormattingEnabled = true;
			this->files->HorizontalScrollbar = true;
			this->files->Location = System::Drawing::Point(12, 40);
			this->files->Name = L"files";
			this->files->Size = System::Drawing::Size(652, 290);
			this->files->TabIndex = 0;
			this->files->DoubleClick += gcnew System::EventHandler(this, &MyForm::files_DoubleClick);
			// 
			// ldList
			// 
			this->ldList->FormattingEnabled = true;
			this->ldList->Location = System::Drawing::Point(12, 12);
			this->ldList->Name = L"ldList";
			this->ldList->Size = System::Drawing::Size(144, 21);
			this->ldList->TabIndex = 1;
			this->ldList->SelectedValueChanged += gcnew System::EventHandler(this, &MyForm::ldList_SelectedValueChanged);
			// 
			// path
			// 
			this->path->Location = System::Drawing::Point(162, 12);
			this->path->Name = L"path";
			this->path->ReadOnly = true;
			this->path->Size = System::Drawing::Size(502, 20);
			this->path->TabIndex = 4;
			// 
			// Refresh
			// 
			this->Refresh->Location = System::Drawing::Point(12, 336);
			this->Refresh->Name = L"Refresh";
			this->Refresh->Size = System::Drawing::Size(100, 36);
			this->Refresh->TabIndex = 5;
			this->Refresh->Text = L"Обновить";
			this->Refresh->UseVisualStyleBackColor = true;
			this->Refresh->Click += gcnew System::EventHandler(this, &MyForm::Refresh_Click);
			// 
			// structs
			// 
			this->structs->AllowUserToOrderColumns = true;
			this->structs->AllowUserToResizeRows = false;
			this->structs->AutoSizeColumnsMode = System::Windows::Forms::DataGridViewAutoSizeColumnsMode::Fill;
			this->structs->BackgroundColor = System::Drawing::SystemColors::ControlLightLight;
			this->structs->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->structs->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(5) {
				this->Disk, this->Part,
					this->Type, this->Size, this->Column1
			});
			this->structs->Location = System::Drawing::Point(13, 40);
			this->structs->MultiSelect = false;
			this->structs->Name = L"structs";
			this->structs->ReadOnly = true;
			this->structs->RowHeadersVisible = false;
			this->structs->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->structs->Size = System::Drawing::Size(651, 290);
			this->structs->TabIndex = 6;
			this->structs->DoubleClick += gcnew System::EventHandler(this, &MyForm::structs_DoubleClick);
			// 
			// Disk
			// 
			this->Disk->HeaderText = L"Диск";
			this->Disk->Name = L"Disk";
			this->Disk->ReadOnly = true;
			// 
			// Part
			// 
			this->Part->HeaderText = L"Раздел";
			this->Part->Name = L"Part";
			this->Part->ReadOnly = true;
			// 
			// Type
			// 
			this->Type->HeaderText = L"Тип";
			this->Type->Name = L"Type";
			this->Type->ReadOnly = true;
			// 
			// Size
			// 
			this->Size->HeaderText = L"Размер (MB)";
			this->Size->Name = L"Size";
			this->Size->ReadOnly = true;
			// 
			// Column1
			// 
			this->Column1->HeaderText = L"Сегментация";
			this->Column1->Name = L"Column1";
			this->Column1->ReadOnly = true;
			// 
			// Details
			// 
			this->Details->Location = System::Drawing::Point(564, 336);
			this->Details->Name = L"Details";
			this->Details->Size = System::Drawing::Size(100, 36);
			this->Details->TabIndex = 7;
			this->Details->Text = L"Подробнее";
			this->Details->UseVisualStyleBackColor = true;
			this->Details->Click += gcnew System::EventHandler(this, &MyForm::Details_Click);
			// 
			// hddCur
			// 
			this->hddCur->AutoSize = true;
			this->hddCur->Location = System::Drawing::Point(204, 15);
			this->hddCur->Name = L"hddCur";
			this->hddCur->Size = System::Drawing::Size(0, 13);
			this->hddCur->TabIndex = 8;
			// 
			// gist
			// 
			chartArea3->Name = L"ChartArea1";
			this->gist->ChartAreas->Add(chartArea3);
			this->gist->Location = System::Drawing::Point(332, 51);
			this->gist->Name = L"gist";
			series3->ChartArea = L"ChartArea1";
			series3->Name = L"ser1";
			this->gist->Series->Add(series3);
			this->gist->Size = System::Drawing::Size(320, 267);
			this->gist->TabIndex = 9;
			this->gist->Text = L"chart1";
			title3->Name = L"Title1";
			title3->Text = L"Использование памяти (МБ)";
			this->gist->Titles->Add(title3);
			// 
			// btRoot
			// 
			this->btRoot->Location = System::Drawing::Point(118, 336);
			this->btRoot->Name = L"btRoot";
			this->btRoot->Size = System::Drawing::Size(100, 36);
			this->btRoot->TabIndex = 10;
			this->btRoot->Text = L"Список дисков";
			this->btRoot->UseVisualStyleBackColor = true;
			this->btRoot->Visible = false;
			this->btRoot->Click += gcnew System::EventHandler(this, &MyForm::btRoot_Click);
			// 
			// btShowEmpty
			// 
			this->btShowEmpty->Location = System::Drawing::Point(431, 336);
			this->btShowEmpty->Name = L"btShowEmpty";
			this->btShowEmpty->Size = System::Drawing::Size(127, 36);
			this->btShowEmpty->TabIndex = 11;
			this->btShowEmpty->Text = L"Пустые директории";
			this->btShowEmpty->UseVisualStyleBackColor = true;
			this->btShowEmpty->Click += gcnew System::EventHandler(this, &MyForm::btShowEmpty_Click);
			// 
			// lbFS
			// 
			this->lbFS->AutoSize = true;
			this->lbFS->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->lbFS->Location = System::Drawing::Point(292, 343);
			this->lbFS->Name = L"lbFS";
			this->lbFS->Size = System::Drawing::Size(0, 20);
			this->lbFS->TabIndex = 12;
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(676, 384);
			this->Controls->Add(this->lbFS);
			this->Controls->Add(this->btShowEmpty);
			this->Controls->Add(this->btRoot);
			this->Controls->Add(this->structs);
			this->Controls->Add(this->hddCur);
			this->Controls->Add(this->Details);
			this->Controls->Add(this->Refresh);
			this->Controls->Add(this->path);
			this->Controls->Add(this->ldList);
			this->Controls->Add(this->gist);
			this->Controls->Add(this->files);
			this->MaximizeBox = false;
			this->Name = L"MyForm";
			this->Text = L"FileManager";
			this->Load += gcnew System::EventHandler(this, &MyForm::Refresh_Click);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->structs))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->gist))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void Refresh_Click(System::Object^  sender, System::EventArgs^  e) {
		if (ldList->Text == "" || ldList->Text == "Все")
		{
			files->Items->Clear();
			structs->Rows->Clear();
			ldList->Items->Clear();
			disks = new LogDisks();
			diskAmount = 0;
			mbrs = new MBR*[20];
			mbrsN = 0;
			gptsN = 0;
			gpts = new GPT*[20];
			curCatPart = new _Catalog();
			curDir = new FileInfo();
			curPath = "";
			GetDrives(); // get Info about existing logical drives to get names
			ReadDrivesStruct(); // analyse disks structure 
			createLDList(); // output to combobox list of logical drives

			outAllDiskShort(); // show start-form
			structs->Show();
		}
		else
		{
			char* ldName = new char[20];
			strcpy(ldName, (char*)(void*)Marshal::StringToHGlobalAnsi((ldList->Text)));
			if (path->Text != "")
			{
				curPath = new char[path->Text->Length];
				strcpy(curPath, (char*)(void*)Marshal::StringToHGlobalAnsi((path->Text)));
			}
			else curPath = "";
			analyseDiskData(curFS, curHddName, ldName, curAbsFirSec);
			//curDir = new FileInfo();
			//readWholeRoot(curHddName, rootCurClust, curAbsFirSec/*, curDir*/);
			//if (curDir != nullptr)
			//{
			//	outCurDir();
			//}
			Refresh->Enabled = true;
			Details->Text = "О диске";
			path->Text = gcnew String(curPath);
		}
		//outAllDiskParts(); // details
	}

	void createLDList()
	{
		ldList->Items->Add("Все");
		for (int i = 0; i < 50; i++)
		{
			if (strlen(disks->disks[i].Name)>0) 
			{
				ldList->Items->Add(gcnew String(disks->disks[i].Name));
			}
		}
		ldList->SelectedIndex = 0;
	}

	int GetDrives() 
	{
		DWORD Disks;
		char DriveName[3];
		Disks = GetLogicalDrives();
		strcpy_s(DriveName, " :");
		int i = 0;
		while (Disks)
		{
			// If this drive present
			if (Disks & 0x00000001)
			{
				DriveName[0] = 'A' + i;
				if (GetDriveType(DriveName) == DRIVE_FIXED || GetDriveType(DriveName) == DRIVE_REMOVABLE)
				{
					//disks->disks[diskAmount] = { 0 };
					strcpy(disks->disks[diskAmount].Name, DriveName);
					strcat(disks->disks[diskAmount].Name, "\\");
					diskAmount++;
				}			
			}
			Disks = Disks >> 1;
			i++;
		}
		// get GUIDs for GPT
		int k = 0;
		char tempGuid[50] = { 0 };
		while (k < diskAmount) // get GUID
		{
			bool res = GetVolumeNameForVolumeMountPoint(disks->disks[k].Name, tempGuid, 50);
			if (res)
			{
				//strcpy(disks->disks[k].id, tempGuid);
				for (int j = 0; j < strlen(tempGuid)-11; j++)
				{
					disks->disks[k].id[j] = toupper(tempGuid[j+10]);
				}
			}
			k++;
		}
		// get serial numbers for MBR
		k = 0;
		DWORD snTemp; // = new char[8];
		UCHAR szFileSys[255],
			szVolNameBuff[255];
		while (k < diskAmount)
		{
			bool res = GetVolumeInformation(disks->disks[k].Name, NULL, 0, &snTemp, NULL, 0, NULL, 0
			);
			if (res)
			{
				disks->disks[k].ser = snTemp;
			}
			k++;
		}
		
		return 0;
	}

	char* getNameBySN(char* sn, WCHAR* hdd)
	{
		bool find = false;
		int i = 0;
		if(strlen(sn)>0)
		while (i < diskAmount)
		{
			DWORD temp = strtoul(sn, NULL, 16);
			if (temp == disks->disks[i].ser)
			{
				disks->disks[i].PhysDrive = new WCHAR[50];
				wcscpy(disks->disks[i].PhysDrive, hdd);
				return disks->disks[i].Name;
			}
			i++;
		}
		// give unic name
		bool isUnic = true; int num = 1;
		char* part = new char[40];
		do {
			isUnic = true;
			strcpy(part, "Anon "); strcat(part, to_string(num++).c_str()); strcat(part, ":\\");
			for (int j = 0; j < diskAmount; j++)
				if (!strcmp(disks->disks[j].Name, part)) isUnic = false;
		} while (!isUnic);
		// add new logic disk to table
		//disks->disks[diskAmount] = { 0 };
		strcpy(disks->disks[diskAmount].Name, part);
		disks->disks[diskAmount].ser = strtoul(sn, NULL, 16);
		disks->disks[diskAmount].PhysDrive = new WCHAR[50];
		wcscpy(disks->disks[diskAmount].PhysDrive, hdd);
		disks->disks[diskAmount].isSupported = false;
		diskAmount++;
		return disks->disks[diskAmount-1].Name;
	}

	char* getNameByGUID(GUID guid, WCHAR* hdd)
	{
		bool find = false;
		int i = 0;
		while (i < diskAmount)
		{
			if (!strcmp(guidToString(guid), disks->disks[i].id))
			{
				disks->disks[i].PhysDrive = new WCHAR[50];
				wcscpy(disks->disks[i].PhysDrive, hdd);
				return disks->disks[i].Name;
			}
			i++;
		}
		// give unic name
		bool isUnic = true; int num = 1;
		char* part = new char[40];
		do {
			isUnic = true;
			strcpy(part, "Anon "); strcat(part, to_string(num++).c_str()); strcat(part, ":\\");
			for (int j = 0; j < diskAmount; j++)
				if (!strcmp(disks->disks[j].Name, part)) isUnic = false;
		} while (!isUnic);
		// add new logic disk to table
//		disks->disks[diskAmount] = { 0 };
		strcpy(disks->disks[diskAmount].Name, part);
		strcpy(disks->disks[diskAmount].id, guidToString(guid));
		disks->disks[diskAmount].PhysDrive = new WCHAR[50];
		wcscpy(disks->disks[diskAmount].PhysDrive, hdd);
		disks->disks[diskAmount].isSupported = false;
		diskAmount++;
		return part;
	}

	private: System::Void ReadDrivesStruct()
	{
		WCHAR disk[50] = { 0 };  
		CHAR cap[200] = { 0 }; 
		for (int i = 0; i < 25; i++)
		{	
			wcscpy(disk, L"\\\\.\\PhysicalDrive");
			wcscat(disk, to_wstring(i).c_str());	
			HANDLE hDisk = CreateFileW((LPCWSTR)disk, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
			if (hDisk!=INVALID_HANDLE_VALUE)
			{
				UCHAR mbr[512];
				DWORD dwRead = 0; 
				SetFilePointer(hDisk, 0, 0, FILE_BEGIN);  
				if (GetLastError() != NO_ERROR) continue;
				if (!ReadFile(hDisk, mbr, SectSize, &dwRead, 0)) continue;
				if (dwRead != SectSize) continue;
				if (mbr[510]!=0x55 || mbr[511]!=0xaa) continue;
				char gptCode[2]; strcpy(gptCode, "ee");
				if (!strcmp(charToHexStr(mbr[450]).c_str(), gptCode) || !strcmp(charToHexStr(mbr[450 + 16]).c_str(), gptCode) ||
					!strcmp(charToHexStr(mbr[450 + 32]).c_str(), gptCode) || !strcmp(charToHexStr(mbr[450 + 48]).c_str(), gptCode))
				{
					gpts[gptsN] = new GPT;
					gpts[gptsN] =  readGPTStruct(hDisk, disk);
					if (gpts[gptsN]!=nullptr) gptsN++;
				}
				else 
				{
					mbrs[mbrsN] = new MBR;
					mbrs[mbrsN]->PhysDrive = new WCHAR[50];
					wcscpy(mbrs[mbrsN]->PhysDrive, disk);
					mbrs[mbrsN]->ptRec = new MBRptRec;
					readMBRSect(0, disk, 0, true, mbrs[mbrsN]->ptRec);
					if (mbrs[mbrsN] != nullptr) mbrsN++;
				}
				CloseHandle(hDisk);
			}
		}
	}

	void outAllDiskShort() 
	{
		WCHAR disk[50] = { 0 };
		CHAR text[200] = { 0 };
		for (int i = 0; i < gptsN; i++)
		for(int j = 0; j < 128; j++)
		{
			char guid[50] = { 0 };
			strcpy(guid, guidToString(gpts[i]->tables[j].GUID_PT));
			if (!strcmp(guid, "{00000000-0000-0000-0000-000000000000}")) continue;		
			DWORD64 amount = gpts[i]->tables[j].AddrEnd - gpts[i]->tables[j].AddrBeg; // blocks amount
			DWORD64 size = (amount * 512 / SectSize) * SectSize / 1024 / 1024;  // MB
			//wcscpy(disk, gpts[i]->PhysDrive); wcscat(disk, L": GPT"); files->Items->Add(gcnew String(disk));
			int k = structs->Rows->Add();
			WCHAR* disk = new WCHAR[50]; wcscpy(disk, gpts[i]->PhysDrive); // wcscat(disk, L" (GPT)");
			structs->Rows[k]->Cells[0]->Value = gcnew String(disk);
			structs->Rows[k]->Cells[1]->Value = gcnew String(gpts[i]->tables[j].Name);
			
			if (!strcmp(guid, "{C12A7328-F81F-11D2-BA4B-00A0C93EC93B}")) strcpy(text, "EFI");
			else if (!strcmp(guid, "{DE94BBA4-06D1-4D40-A16A-BFD50179D6AC}")) strcpy(text, "Раздел восстановления");
			else if (!strcmp(guid, "{EBD0A0A2-B9E5-4433-87C0-68B6B72699C7}")) strcpy(text, "Раздел данных");
			else if (!strcmp(guid, "{E3C9E316-0B5C-4DB8-817D-F92DF00215AE}")) strcpy(text, "MSR (резервный)");
			else strcpy(text, "Другой");
			structs->Rows[k]->Cells[2]->Value = gcnew String(text);
			structs->Rows[k]->Cells[3]->Value = gcnew String(to_string(size).c_str());
			structs->Rows[k]->Cells[4]->Value = gcnew String("GPT");
			//outCurGPTShort(gpts[i]);
		}
		for (int i = 0; i < mbrsN; i++)
		{
			outCurMBRShort(mbrs[i]);
		}
	}

	void outCurMBRShort(MBR *mbr)
	{
		char* text = new char[200];
		WCHAR* disk = new WCHAR[50]; wcscpy(disk, mbr->PhysDrive); // wcscat(disk, L" (MBR)");
		MBRptRec* curPT = mbr->ptRec;
		while (curPT->nextPTRec) 
		{
			int k = structs->Rows->Add();
			structs->Rows[k]->Cells[0]->Value = gcnew String(disk);
			structs->Rows[k]->Cells[1]->Value = gcnew String(curPT->Name);
			if (curPT->isMain) structs->Rows[k]->Cells[2]->Value = gcnew String("Основной");
			else structs->Rows[k]->Cells[2]->Value = gcnew String("Расширенный");
			structs->Rows[k]->Cells[3]->Value = gcnew String(to_string(curPT->Size).c_str());
			structs->Rows[k]->Cells[4]->Value = gcnew String("MBR");
			curPT = curPT->nextPTRec;
		}			
	}

	void outldInfo(char* ldName, WCHAR* diskName, bool isMBR) // out info about 1 logical drive
	{
		 CHAR text[200] = { 0 };
		 int idisk = 0, ild = 0;
		 files->Items->Clear();
		 DWORD64 size, firstSect, freeSize;
		 if (!isMBR)
		 {
			 while (wcscmp(gpts[idisk]->PhysDrive, diskName)) idisk++;
			 while (strcmp(gpts[idisk]->tables[ild].Name, ldName)) ild++;
			 WCHAR* disk = new WCHAR[50]; wcscpy(disk, diskName); files->Items->Add(gcnew String(disk));
			 outCurGPT(gpts[idisk], ild);
			 firstSect = gpts[idisk]->tables[ild].AddrBeg;
			 size = (gpts[idisk]->tables[ild].AddrEnd - gpts[idisk]->tables[ild].AddrBeg)*SectSize / (1024 * 1024);
		 }
		 else
		 {			
			while (wcscmp(mbrs[idisk]->PhysDrive, diskName)) idisk++;
			MBRptRec *mbr = mbrs[idisk]->ptRec;
			while (strcmp(mbr->Name, ldName)) 
			{
				mbr = mbr->nextPTRec;
			}
			WCHAR* disk = new WCHAR[50]; wcscpy(disk, diskName); files->Items->Add(gcnew String(disk));
			files->Items->Add(gcnew String(text));
			outCurMBR(mbr);
			firstSect = mbr->AbsSect;
			size = mbr->Size;
		 }
		 int fs = readBoot(diskName, firstSect);	 
		 switch (fs)
		 {
		 case 32: lbFS->Text = "FAT32"; break;
		 case 16: lbFS->Text = "FAT16"; break;
		 case 12: lbFS->Text = "FAT12"; break;
		 case 1: lbFS->Text = "NTFS"; break;
		 }
		 if (fs == 32 && isLogDiskSupported(ldName))
		 {
			 size = (curBoot->ldSize - curBoot->reservSize - curBoot->kFat*curBoot->fatSize); // size in sectors
			 size = size * curBoot->sectSize / (1024 * 1024);
			 readFAT(diskName, firstSect);
			 freeSize = getFreeSpaceByCurFAT();
			 char *txt = new char[100];
			 strcpy(txt, "Всего в области данных: "); strcat(txt, to_string(size).c_str()); strcat(txt, " MB");
			 files->Items->Add(gcnew String(txt));
		 }
		 else if(isLogDiskSupported(ldName) && fs!=0)
		 {
			 unsigned __int64 i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
			 BOOL GetDiskFreeSpaceFlag = GetDiskFreeSpaceEx(ldName, // directory name
							(PULARGE_INTEGER)&i64FreeBytesToCaller,
							(PULARGE_INTEGER)&i64TotalBytes,
							(PULARGE_INTEGER)&freeSize );
			 size = i64TotalBytes / (1024*1024);
		 }
		 else freeSize = -1025;
		 if (freeSize != -1025)
		 {
			 freeSize = freeSize / 1024 / 1024;
			 char *txt = new char[100];
			 strcpy(txt, "Свободно на диске: "); strcat(txt, to_string(freeSize).c_str()); strcat(txt, " MB");
			 files->Items->Add(gcnew String(txt)); 
		 }
		 fillGraph(size, freeSize);
	}

	void fillGraph(int allMB, int freeMB) 
	{
		//gist->Series->Clear();
		gist->Show();
		Refresh->Enabled = false;
		if (allMB > 10000) 
		{
			allMB /= 1024;
			freeMB /= 1024;
			gist->Titles[0]->Text = gcnew String("Использование памяти (GB)");
		}
		else gist->Titles[0]->Text = gcnew String("Использование памяти (MB)");
		gist->Series[0]->Points->Clear();
		//DataVisualization::Charting::Series ^ser1 = gcnew DataVisualization::Charting::Series();
		array<int,3> y = { allMB, freeMB, allMB - freeMB };
		array<string,3> x { "Всего", "Свободно", "Занято" };
		for (int i = 0; i < 3; i++)
		{
			gist->Series[0]->Points->AddXY(gcnew String(x[i].c_str()), y[i]);
			if (freeMB < 0) break;
		}
		//ser1->Name = gcnew String(disk);
		//gist->Series->Add(ser1);
		gist->Update();
	}

	DWORD64 getFreeSpaceByCurFAT() 
	{
		DWORD64 size = 0;
		DWORD64 fatlen = (curBoot->ldSize - curBoot->reservSize - curBoot->kFat*curBoot->fatSize)
			/ curBoot->clustSize; // -> number of clusters
		for (DWORD64 i = 2; i < (fatlen+2); i++)
			if (FAT[i] == 0x0) size++; //in clusters
		size *= curBoot->clustSize*curBoot->sectSize; // in bytes
		return size;
	}

	GPT* readGPTStruct(HANDLE hDisk, WCHAR* diskName)
	{
		DWORD dwRead = 0;
		char guid[50] = { 0 };
		GPT* tempGPT = new GPT();
		if (!ReadFile(hDisk, &tempGPT->hpt, SectSize, &dwRead, 0)) return nullptr;
		if (dwRead != SectSize) return nullptr;	
		tempGPT->PhysDrive = new WCHAR[50];
		wcscpy(tempGPT->PhysDrive, diskName);

		int size = tempGPT->hpt.size_PT*tempGPT->hpt.amount_PT; // whole PT size
		if (!ReadFile(hDisk, tempGPT->tables, size, &dwRead, 0)) return nullptr;
		if(dwRead!=size) return nullptr;
		for (int j = 0; j < tempGPT->hpt.amount_PT; j++)
		{
			strcpy(guid, guidToString(tempGPT->tables[j].GUID_PT));
			if (strcmp(guid, "{00000000-0000-0000-0000-000000000000}"))
			strcpy(tempGPT->tables[j].Name, getNameByGUID(tempGPT->tables[j].ID, tempGPT->PhysDrive));
		}
		return tempGPT;
	}

	void outCurGPT(GPT* curGPT, int ild)
	{
		string  parts[2][4] = { { "{C12A7328-F81F-11D2-BA4B-00A0C93EC93B}",
					"{DE94BBA4-06D1-4D40-A16A-BFD50179D6AC}","{EBD0A0A2-B9E5-4433-87C0-68B6B72699C7}",
					"{E3C9E316-0B5C-4DB8-817D-F92DF00215AE}" },
			{" - EFI"," - Раздел восстановления"," - Раздел данных"," - MSR (резервный)"} };
		char guid[50] = { 0 }, cap[200] = { 0 };
		strcpy(guid, guidToString(curGPT->hpt.Dev_GUID));
		strcpy(cap, "GUID диска = ");  strcat(cap, guid); 
		files->Items->Add(gcnew String(cap));
		int i = ild;
			strcpy(guid, guidToString(curGPT->tables[i].GUID_PT));
			if (strcmp(guid, "{00000000-0000-0000-0000-000000000000}"))
			{
				strcpy(cap, "GUID типа раздела: "); files->Items->Add(gcnew String(cap)); 
				strcpy(cap, guid); 
				for (int k = 0; k < 4; k++)
					if (!strcmp(guid, parts[0][k].c_str())) {
						strcat(cap, parts[1][k].c_str());
						break;
					}
				files->Items->Add(gcnew String(cap));
				strcpy(guid, guidToString(curGPT->tables[i].ID));
				strcpy(cap, "GUID раздела = ");  strcat(cap, guid);
				files->Items->Add(gcnew String(cap));
				strcpy(cap, "Имя раздела = ");  strcat(cap, curGPT->tables[i].Name);
				files->Items->Add(gcnew String(cap));

				strcpy(cap, "Начало раздела (блок №) = ");
				strcat(cap, to_string(curGPT->tables[i].AddrBeg).c_str()); files->Items->Add(gcnew String(cap));
				
				DWORD64 amount = curGPT->tables[i].AddrEnd - curGPT->tables[i].AddrBeg; // blocks amount
				DWORD64 size = (amount * 512 / SectSize) * SectSize / 1024 / 1024;  // MB
				strcpy(cap, "Размер раздела = ");
				strcat(cap, to_string(size).c_str());
				strcat(cap, " MB");
				if (size > 9999)
				{
					strcat(cap, " = "); strcat(cap, to_string(size / 1024).c_str());
					strcat(cap, " GB");
				}
				files->Items->Add(gcnew String(cap));
				files->Items->Add(gcnew String(""));
			}
	}

	// sectBeg - MBR[k]Addr, addr_beg - MBR[0]Addr
	MBRptRec* readMBRSect(UINT sectBeg, WCHAR* diskName, UINT addr_beg, bool isMain, MBRptRec *pt)
	{
		HANDLE hDisk = CreateFileW((LPCWSTR)diskName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
		if (hDisk != INVALID_HANDLE_VALUE)
		{
			UCHAR mbr[512];
			DWORD dwRead = 0;
			UINT secBegin, RecSizeInSect, mbLength = 0, gbLength = 0;
			LARGE_INTEGER  li;
			li.QuadPart = (LONGLONG)sectBeg * SectSize;
			UINT res = SetFilePointer(hDisk, li.LowPart, &li.HighPart, FILE_BEGIN);
			if (!ReadFile(hDisk, mbr, SectSize, &dwRead, 0)) return nullptr;
			if (dwRead != 512) return nullptr;
			if (mbr[510] != 0x55 || mbr[511] != 0xaa) return nullptr;
			UCHAR* ptRec = mbr + 0x01BE;
			MBRptRec *curPT = pt;
			for (int i = 0; i < 4; i++)
			{
				int sysCode = ptRec[4];
				if (sysCode)
				{
					secBegin = *(UINT*)&ptRec[8]; //OtnAddr
					if (sysCode == 0x5 || sysCode == 0xf)
					{
						if (isMain) curPT =  // otnAddr[k]+otnAddr[0]
							readMBRSect(secBegin + addr_beg, diskName, secBegin, false, curPT);
						else curPT = readMBRSect(secBegin + addr_beg, diskName, addr_beg, false, curPT);
					}
					else
					{
						curPT->sysCode = sysCode;
						curPT->isMain = isMain;
						if (ptRec[0]) curPT->isBoot = true;
						else curPT->isBoot = false;
						curPT->BegAddr = secBegin;
						curPT->AbsSect = secBegin + sectBeg; // OtnAddr + MBR[k]Addr
						RecSizeInSect = *(UINT*)&ptRec[12];	// size in sectors

						mbLength = RecSizeInSect / (1024 * 1024 / SectSize);
						curPT->Size = mbLength;
						curPT->serNum = readSerNum(sysCode, curPT->AbsSect, diskName);
						char* tempName = new char[20]; strcpy(tempName, getNameBySN(getStrSerNum(curPT->serNum), diskName));
						curPT->Name = new char[strlen(tempName)];
						strcpy(curPT->Name, tempName);

						curPT->nextPTRec = new MBRptRec;
						curPT = curPT->nextPTRec;
						curPT->nextPTRec = nullptr;
					}
				}
				ptRec += 0x10;
			}
			CloseHandle(hDisk);
			return curPT;
		}
		return nullptr;
	}

	void outCurMBR(MBRptRec *curPT)
	{
		char* text = new char[200];
		strcpy(text, "Имя диска: ");  strcat(text, curPT->Name); files->Items->Add(gcnew String(text));
		strcpy(text, "Тип раздела: ");
		if (curPT->isMain) strcat(text, "Основной"); else strcat(text, "Расширенный");
		files->Items->Add(gcnew String(text));
		strcpy(text, "Код системы: "); strcat(text, to_string(curPT->sysCode).c_str()); 
		strcat(text, " ("); strcat(text, getFileSysStr(curPT->sysCode)); strcat(text, ")");
		files->Items->Add(gcnew String(text));
		if (curPT->isBoot) strcpy(text, "Загрузочный"); else strcpy(text, "Не загрузочный");
		files->Items->Add(gcnew String(text));
		strcpy(text, "Сектор: "); strcat(text, to_string(curPT->AbsSect).c_str()); 
		files->Items->Add(gcnew String(text));
		strcpy(text, "Размер (МБ): "); strcat(text, to_string(curPT->Size).c_str());
		files->Items->Add(gcnew String(text));
		strcpy(text, "Серийный номер: "); strcat(text, getStrSerNum(curPT->serNum)); 
		files->Items->Add(gcnew String(text));
	}

char* getStrSerNum(UCHAR* iSer)
{
	std::string ser(reinterpret_cast<char*>(iSer));
	char *sn = new char[8]; strcpy(sn, "");
	if(strlen(ser.c_str())>2)
	for (int i = 3; i >= 0; i--)
	{
		string s = charToHexStr(ser[i]);
		strcat(sn, s.c_str());
	}
	return sn;
}

char* guidToString(GUID guid) {
    char* output = new char[50];
	char szGuid[50] = { 0 };
	sprintf(szGuid, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", (guid).Data1,
		(guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3],
		(guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]);
	strcpy(output, szGuid);
	return output;
}

string charToHexStr(unsigned char inchar)
{
	ostringstream oss(ostringstream::out);	
	oss << setw(2) << setfill('0') << hex << (int)(inchar);
	return oss.str();
}

char* getFileSysStr(unsigned char code)
{
	if ((code == 0x07) || (code == 0x17))
		return "NTFS";
	else if ((code == 0x0B) || (code == 0x0C))
		return "FAT32";
	else if ((code == 0x0E) || (code==0x06) || (code==0x04))
		return "FAT16";
	else if (code == 0x01)
		return "FAT12";
	else return "Other";
}

UCHAR* readSerNum(UCHAR sysCode, UINT absAddr, WCHAR* diskName)
{
	UCHAR* SerNum = new UCHAR[20];
	UCHAR rec[512];
	DWORD dwRead = 0;
	LARGE_INTEGER  li;
	li.QuadPart = (LONGLONG)absAddr * SectSize;
	memcpy(SerNum, "\0", 1);
	HANDLE hDisk = CreateFileW((LPCWSTR)diskName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hDisk != INVALID_HANDLE_VALUE) 
	{
		int res = SetFilePointer(hDisk, li.LowPart, &li.HighPart, FILE_BEGIN);
		if(!ReadFile(hDisk, rec, SectSize, &dwRead, 0)) return SerNum;
		if (dwRead != 512) return SerNum;
		if (rec[510]!=0x55 || rec[511]!=0xaa) return SerNum;
		switch (sysCode)
		{
		case 0x07:			//NTFS
			memcpy(SerNum, rec + 72, 8);
			break;
		case 0x0C:
		case 0x0B:			//FAT32
			memcpy(SerNum, rec + 67, 4);
			break;
		case 0x06:
		case 0x0E:		//FAT16
			memcpy(SerNum, rec + 39, 4); break;
		default:
			memcpy(SerNum, "\0", 1);
		}
		CloseHandle(hDisk);
	}
	return SerNum;
}

// return 32 for fat32, 16 for fat16, 12 for fat12, 1 - NTFS, 2 - other, 0 - cannot read sector
int readBoot(WCHAR* diskName, DWORD64 FirstSectNum)
{
	int fs = 2;
	UCHAR boot[512];
	DWORD dwRead = 0;
	LARGE_INTEGER  li;
	li.QuadPart = (LONGLONG)FirstSectNum * SectSize;
	HANDLE hDisk = CreateFileW((LPCWSTR)diskName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hDisk != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hDisk, li.LowPart, &li.HighPart, FILE_BEGIN);
		if (GetLastError() != NO_ERROR || !ReadFile(hDisk, boot, SectSize, &dwRead, 0)) return 0;
		if (dwRead != SectSize || boot[510] != 0x55 || boot[511] != 0xaa) return 0;	
		CHAR*ntfs = new CHAR[8];
		strcpy(ntfs, (CHAR*)&boot[3]);
		if (!strcmp("NTFS    ", ntfs)) return 1;
		curBoot->sectSize = *(UINT*)&boot[11];
		curBoot->clustSize = *(UINT*)&boot[13];
		curBoot->reservSize = *(UINT*)&boot[14]; 
		curBoot->kFat = *(UINT*)&boot[16];
		curBoot->maxRootRecCount = *(UINT*)&boot[17];
		WORD temp = *(UINT*)&boot[19];
		if(temp!=0) curBoot->ldSize = temp;
		else curBoot->ldSize = *(UINT*)&boot[32];
		WORD temp2 = *(UINT*)&boot[22];
		
		if (temp2 != 0) curBoot->fatSize = temp2;//1st model
		else curBoot->fatSize = *(UINT*)&boot[36]; // fat32

		/*if(temp!=0) curBoot->fatSize = temp;
		else curBoot->fatSize = *(UINT*)&boot[36];*/
		curBoot->sizeServ = curBoot->reservSize + curBoot->fatSize*curBoot->kFat + 
						curBoot->maxRootRecCount*32/curBoot->sectSize;
		DWORD clustAmount = (curBoot->ldSize - curBoot->sizeServ) / curBoot->clustSize;
		if (curBoot->maxRootRecCount != 0 && temp2 != 0 && clustAmount < 4085) fs = 12;
		else if (curBoot->maxRootRecCount != 0 && temp2 != 0 && clustAmount <= 65525) fs = 16;
		else if (curBoot->maxRootRecCount == 0 && temp == 0 && clustAmount > 65525) fs = 32;
		if (fs == 32)
		{
			curBoot->rootFirstClust = *(UINT*)&boot[44];
			curBoot->RootAddr = FirstSectNum + (curBoot->rootFirstClust-2)*curBoot->clustSize + curBoot->sizeServ;
			curBoot->RootSize = curBoot->clustSize;
		}
		CloseHandle(hDisk);
		return fs;
	}
	else return 0;
}

void renewCurDir(WCHAR* diskName, char* ldName, DWORD64 FirstSecNum)
{
	if (readFAT(diskName, curAbsFirSec))
	{
		//readRoot(diskName, curBoot->RootAddr, curBoot->rootFirstClust, FirstSecNum, curDir, false);
		delete curDir;
		curDir = new FileInfo();
		readWholeRoot(diskName, rootCurClust, /*curBoot->rootFirstClust*/ FirstSecNum/*, curDir*/);
		if (curDir != nullptr)
		{
			outCurDir(nullptr);
		}
	}
}

void ntfsOpenDir(string path)
{
	//vector<string> names;
	char* txt = new char[300];
	//string search_path = path + "/*.*";
	string search_path = path + "*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	files->Items->Clear();
	curDir = new FileInfo();
	FileInfo* p = curDir; //?
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			//if (/*!*/(fd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY || FILE_ATTRIBUTE_ARCHIVE))) 
			//{
				p->cname = new char[255];
				strcpy(p->cname, fd.cFileName);
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) p->type = 0;
				else if(fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) p->type = 1;
			//	else p->type = -1;
				p->nextRec = new FileInfo();
				p = p->nextRec;
			//}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	else
	{
		p->cname = new char[4];
		strcpy(p->cname, "..");
		p->nextRec = new FileInfo();
		p = p->nextRec;
		p->cname = new char[30];
		strcpy(p->cname, "Отказано в доступе");
		p->type = -1;
		p->nextRec = new FileInfo();
	}
	//return names;
}

void analyseDiskData(int fs, WCHAR* diskName, char* ldName, DWORD64 FirstSecNum)
{
	files->Items->Clear();
	structs->Hide(); gist->Hide();
	//char *txt = new char[path->Text->Length];
	string txt;
	switch (fs)
	{
	case 0: 
		lbFS->Text = "";
		files->Items->Add("К сожалению, возникли ошибки при чтении диска");  
		break;
	case 1: 
		lbFS->Text = "NTFS";
		//strcpy(txt, (char*)(void*)Marshal::StringToHGlobalAnsi(path->Text));
		txt = msclr::interop::marshal_as<std::string>(path->Text);
		ntfsOpenDir(txt);
		outCurDir(nullptr);
		//files->Items->Add("ФС NTFS пока не поддерживается"); 
		break;
	case 32: 
		lbFS->Text = "FAT32";
		renewCurDir(diskName, ldName, FirstSecNum);
		btShowEmpty->Enabled = true;
		break;
	default: 
		if (fs==12) files->Items->Add("ФС: FAT12");
		else if(fs==16) files->Items->Add("ФС: FAT16");
		else files->Items->Add("Неизвестная ФС");
		files->Items->Add("Эта ФС поддерживаться не будет");
		break;
	}
	if(fs!=32) btShowEmpty->Enabled = false;
}

DWORD getClustChain(DWORD64 rootCurClust, _NumbersChain* clusts)
{
	DWORD amount = 1;
	_NumbersChain *cls = clusts;
	cls->cl = rootCurClust;
	while (1)
	{
		if (FAT[cls->cl] != 0x0 && FAT[cls->cl] >= 0x2 && FAT[cls->cl] <= 0xff && 
			!(FAT[cls->cl]>=0xf0 && FAT[cls->cl]<=0xf6))
		{
			cls->next = new _NumbersChain;
			cls->next->cl = FAT[cls->cl];
			cls = cls->next;
			amount++;
		}
		else return amount;
	}
}

UCHAR ror(UCHAR a)
{
	byte last = a & 1;
	UCHAR temp = a >> 1;
	byte first = last << 7; 
	UCHAR res = first | temp;
	return res;
	/*UCHAR t1, t2;
	n = n % (sizeof(a) * 8);
	t1 = a >> 1;
	t2 = a << sizeof((a) * 8 - n);
	return t1 | t2;*/
}

//uint32_t rotr32(uint32_t n, unsigned int c)
//{
//	const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
//	c &= mask;  
//	return (n >> c) | (n << ((-c)&mask));
//}

DWORD64 readWholeRoot(WCHAR* diskName, long rootCurClust, DWORD64 FirstSectNum/*, FileInfo* curDir*/)
{
	DWORD64 recAmount = 0;
	DWORD64 rootCurSect;
	DWORD dwRead = 0;
	DWORD64 clustInBytes = curBoot->clustSize*curBoot->sectSize;// размер 1 кластера в байтах
	_NumbersChain *clusts = new _NumbersChain;  // цепочка кластеров
	DWORD64 clusCount = getClustChain(rootCurClust, clusts); // количество занимаемых кластеров
	DWORD64 rows = clusCount*clustInBytes / 32; // дескрипторов всего в каталоге
	curCatPart->descs = new Descriptor[rows];
	Descriptor* curCat = curCatPart->descs;
	LARGE_INTEGER  li;	
	HANDLE hDisk = CreateFileW((LPCWSTR)diskName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hDisk != INVALID_HANDLE_VALUE)
	{
		bool res = true;
		for (DWORD i = 0; i < clusCount; i++)
		{
			rootCurClust = clusts->cl;
			clusts = clusts->next;
			rootCurSect = FirstSectNum + (rootCurClust - 2)*curBoot->clustSize + curBoot->sizeServ;
			li.QuadPart = rootCurSect * curBoot->sectSize;
			SetFilePointer(hDisk, li.LowPart, &li.HighPart, FILE_BEGIN);
			if (GetLastError() != NO_ERROR ||
				!ReadFile(hDisk, curCat, clustInBytes, &dwRead, 0))
				return 0;
			if (dwRead != clustInBytes) return 0;
			curCat = &curCatPart->descs[(clustInBytes/32)*(i+1)];
		}
		CloseHandle(hDisk);
		DWORD64 i = 0;
		FileInfo *next = curDir;
		while(i<rows)
		{
			int attr = curCatPart->descs[i].Attr;
			if (curCatPart->descs[i].Name[0] == 0x0) return recAmount;	// end of catalog
			if (curCatPart->descs[i].Name[0] == 0xe5 || 
				curCatPart->descs[i].Name[0] == 0x05) { i++;  continue; } // this object was deleted
			if (attr == 0x8) { i++;  continue; } // metka toma 	
			if (attr == 0x0F)
			{
				if (curCatPart->descs[i].Name[0] > 0x40) // this is last part 
				{ 
					DWORD length = curCatPart->descs[i].Name[0] & 0x3f; // & 0011 1111
					i += length; // i - index of base long descriptor
					WCHAR *lName = new WCHAR[255];
					int pos = 0;
					res = true;
					if (i>=rows || curCatPart->descs[i].Name[0] == 0x0) return recAmount;
					if (curCatPart->descs[i].Name[0] == 0xe5)  { i++; continue; }// filed was deleted
					// get control sum

					UCHAR ks = 0; 
					UCHAR* nm = new UCHAR[12];
					memcpy(nm, curCatPart->descs[i].Name, 8);
					memcpy(nm+8, curCatPart->descs[i].Ext, 3);
					nm[11] = '\0';

					for (int j = 0; j < 11; j++)
					{	
						ks = ror(ks);
						ks += nm[j];
					}

					for (int k = 0; k < length; k++, pos += 13)
					{
						UCHAR ksLN = (UCHAR)curCatPart->descs[i - 1 - k].Other[1];
						if (curCatPart->descs[i - 1 - k].Attr == 0x0F && ks == ksLN)
						{
							memcpy(lName + pos, (WCHAR *)&curCatPart->descs[i - 1 - k].Name[1], 10);
							memcpy(lName + pos + 5, (WCHAR *)&curCatPart->descs[i - 1 - k].Other[2], 12);
							memcpy(lName + pos + 11, (WCHAR *)&curCatPart->descs[i - 1 - k].Size, 4);
						}
						else
						{
							res = false;
							next->wname = nullptr;
							break;
						}
					}
					if (res) // got long name
					{
						lName[pos] = L'\0';
						next->wname = new WCHAR[255];
						wcscpy(next->wname, lName);
					}
				}
				else { i++; continue; }// strange record,  leave it
			} // if not 0x0f or later:
			if (attr !=0xf || !res) // could not read long name or did not have long name
			{
				next->cname = new char[13];
				memcpy(next->cname, curCatPart->descs[i].Name, 8);
				int k = 8; next->cname[k--] = '\0';
				while (next->cname[k]==' ') next->cname[k--] = '\0';
				if (curCatPart->descs[i].Ext[0] != ' ')
				{
					memcpy(next->cname+k+1, ".", 1);
					memcpy(next->cname+k+2, curCatPart->descs[i].Ext, 3);
					//next->cname[12] = '\0';
					k += 5; next->cname[k--] = '\0';
					while (next->cname[k] == ' ') next->cname[k--] = '\0';
				}
			}
			if (curCatPart->descs[i].Attr & 0x20) next->type = 1;
			else if (curCatPart->descs[i].Attr & 0x10) next->type = 0;
			DWORD firstClLo = curCatPart->descs[i].EndClust;
			DWORD firstClHi = curCatPart->descs[i].BegClust;
			next->cluster = firstClHi;
			next->cluster = (next->cluster << 16) + firstClLo;
			next->size = curCatPart->descs[i].Size;
			next->nextRec = new FileInfo();
			next = next->nextRec;
			recAmount++;
			i++;
		} /*while (i<rows)*/
		delete[] curCatPart->descs;
		return recAmount;
	}
	else 
	delete[] curCatPart->descs;
	return 0;
}

void findEmptyDirs(_NumbersChain* ind)
{
	FileInfo* curDirSave = curDir;
	FileInfo *fi = new FileInfo();
	fi = curDir;
	int i = 0;
	while (fi->nextRec != nullptr)
	{
		curDir = new FileInfo();
		// для каждой директории из текущего каталога кроме '.' и '..'
		if (!fi->type && (!fi->cname || strcmp(fi->cname,".") && strcmp(fi->cname, ".."))) 
		{
			DWORD64 res;
		//	if (fi->cluster != 0)	// we choose whole drive C:\, D:\ or other
			res = readWholeRoot(curHddName, fi->cluster, curAbsFirSec);
			//else					// we choose directory
			//	readWholeRoot(curHddName, curBoot->rootFirstClust, curAbsFirSec);
			if (res == 2) // empty directory number fi->ind
			{
				ind->cl = fi->ind;
				ind->next = new _NumbersChain;
				ind = ind->next;
			}
		}
		fi = fi->nextRec;
		delete[] curDir; 
	}
	curDir = curDirSave;
}

void outCurDir(_NumbersChain* ind)
{
	FileInfo *fi = new FileInfo();
	fi = curDir;
	int i = 0;
	char* txt = new char[300];
	WCHAR* wtxt = new WCHAR[300];
	files->Items->Clear();
	while (fi->nextRec!=nullptr)
	{
		fi->ind = i++;
		if (fi->wname) 
		{
			if(fi->type==1) wcscpy(wtxt, L"file\t");
			else if(fi->type==0) wcscpy(wtxt, L"dir\t");
			else wcscpy(wtxt, L"\t");
			if (ind && ind->cl == fi->ind)
			{
				wcscat(wtxt, L"ПУСТАЯ\t\t");
				ind = ind->next;
			}
			else wcscat(wtxt, L"\t\t");
			wcscat(wtxt, fi->wname);
			files->Items->Add(gcnew String(wtxt));
		}
		else 
		{
			if (fi->type==1) strcpy(txt, "file\t");
			else if(fi->type==0) strcpy(txt, "dir\t");
			else strcpy(txt, "\t");
			if (ind && ind->cl == fi->ind)
			{
				strcat(txt, "ПУСТАЯ\t\t");
				ind = ind->next;
			}
			else strcat(txt, "\t\t");
			strcat(txt, fi->cname);
			files->Items->Add(gcnew String(txt));
		}
		fi = fi->nextRec;
	}
}

DWORD getNextCatalClust(long clust)
{
	if (FAT[clust] > 1 && FAT[clust] < 0xfffffff) return FAT[clust];
	else return 0;
}

bool readFAT(WCHAR* diskName, DWORD64 FirstSecNum)
{
	DWORD64 fatlen = curBoot->fatSize*curBoot->sectSize; // in bytes *curBoot->kFat
	LARGE_INTEGER li; // start of FAT
	li.QuadPart = (FirstSecNum + curBoot->reservSize)*curBoot->sectSize;
	delete[] FAT;
	FAT = new DWORD[fatlen/4];
	//UCHAR *FAT = new UCHAR[fatlen];
	DWORD dwRead = 0;
	HANDLE hDisk = CreateFileW((LPCWSTR)diskName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hDisk != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hDisk, li.LowPart, &li.HighPart, FILE_BEGIN);
		if (GetLastError() != NO_ERROR || !ReadFile(hDisk, FAT, fatlen, &dwRead, 0)) return 0;
		if (dwRead != fatlen) return 0;
		return 1;
		CloseHandle(hDisk);
	}
	else return 0;
}

bool isLogDiskSupported(char* name)
{
	for (int i = 0; i < diskAmount; i++)
	{
		if (!strcmp(disks->disks[i].Name, name))
		{
			if (disks->disks[i].isSupported) return true;
			else return false;
		}
	}
}

private: System::Void Details_Click(System::Object^  sender, System::EventArgs^  e) {

	char* ldName = new char[20];
	strcpy(ldName, (char*)(void*)Marshal::StringToHGlobalAnsi((ldList->Text)));
	if (structs->Visible && ldList->Text == "Все") // show disk info for selected disk
	{
		if (structs->SelectedCells[0]->RowIndex == structs->Rows->Count - 1) return;
		structs->Hide();
		files->Items->Clear();
		btShowEmpty->Enabled = false;
		Details->Text = "Вернуться";
		char*diskName = new char[30];
		strcpy(diskName, (char*)(void*)Marshal::StringToHGlobalAnsi(structs->SelectedRows[0]->Cells[1]->Value->ToString()));
		char* typeHdd = new char[10];
		strcpy(typeHdd, (char*)(void*)Marshal::StringToHGlobalAnsi(structs->SelectedRows[0]->Cells[4]->Value->ToString()));
		WCHAR* disk = new WCHAR[20];
		wcscpy(disk, (WCHAR*)(void*)Marshal::StringToHGlobalUni(structs->SelectedRows[0]->Cells[0]->Value->ToString()));
		outldInfo(diskName, disk, !strcmp(typeHdd, "MBR"));
	}
	else if (ldList->Text != "Все" && Details->Text=="О диске") // show disk info for curDisk
	{
		curPath = new char[path->Text->Length];
		strcpy(curPath, (char*)(void*)Marshal::StringToHGlobalAnsi((path->Text)));
		outldInfo(ldName, curHddName, isMBR);
		Details->Text = "Вернуться";
		btShowEmpty->Enabled = false;
	}
	else if (ldList->Text == "Все") // return to main
	{
		structs->Visible = true;
		Refresh->Enabled = true;
		Details->Text = "О диске";
		btShowEmpty->Enabled = false;
		lbFS->Text = "";
	}
	else // return to curDisk
	{
		/*outCurDir(nullptr);*/
		analyseDiskData(curFS, curHddName, ldName, curAbsFirSec);
		Refresh->Enabled = true;
		Details->Text = "О диске";
		path->Text = gcnew String(curPath);
		if(curFS==32) btShowEmpty->Enabled = true;
	}

}

private: System::Void ldList_SelectedValueChanged(System::Object^  sender, System::EventArgs^  e) {
	char* ldName = new char[20];
	int fs = -1;
	path->Text = "";
	curPath = "";
	strcpy(ldName, (char*)(void*)Marshal::StringToHGlobalAnsi((ldList->Text)));
	Details->Text = "О диске";
	if (!strcmp(ldName, "Все")) // show list of disks
	{
		structs->Show(); 
		gist->Hide(); //
		btRoot->Visible = false;
		btShowEmpty->Enabled = false;
		lbFS->Text = "";
		return;
	}
	path->Text += gcnew String(ldName);
	//btShowEmpty->Enabled = true;
	btRoot->Visible = true;
	for (int i = 0; i < gptsN && fs==-1; i++)
	for(int j = 0; j<128 && fs == -1; j++)
	{
		if (!strcmp(gpts[i]->tables[j].Name, ldName))
		{
			if (!isLogDiskSupported(ldName)) fs = 2;
			else fs = readBoot(gpts[i]->PhysDrive, gpts[i]->tables[j].AddrBeg);
			wcscpy(curHddName, gpts[i]->PhysDrive);
			isMBR = false;
			curAbsFirSec = gpts[i]->tables[j].AddrBeg;
			curFS = fs;
			rootCurClust = curBoot->rootFirstClust;
			analyseDiskData(fs, gpts[i]->PhysDrive, gpts[i]->tables[j].Name, gpts[i]->tables[j].AddrBeg);
		}
	}
	for (int i = 0; i < mbrsN, fs == -1; i++)
	{
		WCHAR* diskName = mbrs[i]->PhysDrive;
		MBRptRec* rec = new MBRptRec;
		rec = mbrs[i]->ptRec;
		while (rec->nextPTRec != nullptr && fs == -1)
		{
			if (!strcmp(rec->Name, ldName)) 
			{
				if (!isLogDiskSupported(ldName)) fs = 2;
				else fs = readBoot(mbrs[i]->PhysDrive, rec->AbsSect);
				wcscpy(curHddName, mbrs[i]->PhysDrive);
				isMBR = true;
				curAbsFirSec = rec->AbsSect;
				curFS = fs;
				rootCurClust = curBoot->rootFirstClust;
				analyseDiskData(fs, mbrs[i]->PhysDrive, rec->Name, rec->AbsSect);
			}		 
			rec = rec->nextPTRec;
		}
	}
	//path->Text += gcnew String(ldName);
}
private: System::Void structs_DoubleClick(System::Object^  sender, System::EventArgs^  e) {
	char*diskName = new char[30];
	if (structs->SelectedCells[0]->RowIndex == structs->Rows->Count - 1) return;
	strcpy(diskName, (char*)(void*)Marshal::StringToHGlobalAnsi(structs->SelectedRows[0]->Cells[1]->Value->ToString()));
	ldList->Text = gcnew String(diskName);
//	ldList_SelectedValueChanged(sender, e);
}
private: System::Void btRoot_Click(System::Object^  sender, System::EventArgs^  e) {
	structs->Show();
	Refresh->Enabled = true;
	path->Text = "";
	gist->Hide(); //
	btRoot->Visible = false;
	Details->Text = "О диске";
	ldList->Text = "Все";
	btShowEmpty->Enabled = false;
}
private: System::Void files_DoubleClick(System::Object^  sender, System::EventArgs^  e) {
	int num = files->SelectedIndex;
	if (num == -1) return;
	FileInfo* p = curDir;

	if (curFS != 32 && curFS!=1) return;
	while (p->nextRec != nullptr) // find record in list
	{
		if (p->ind == num) break;
		p = p->nextRec;
	}
	if (p->type == 0 && (!p->cname || strcmp(p->cname, "."))) // dir and not '.'
	{
		if (!p->cname || strcmp(p->cname, "..")) // open next dir
		{
			char *nm = new char[300];
			if (p->wname)
				path->Text += gcnew String(p->wname); //strcpy(nm, p->cname);
			else
			{
				path->Text += gcnew String(p->cname);
			}
			path->Text += "\\";
		} // else - return back, '..' pressed
		else{
			char *txt = new char[path->Text->Length*2];
			strcpy(txt, (char*)(void*)Marshal::StringToHGlobalAnsi(path->Text));
			int i = strlen(txt) - 1; txt[i--] = '\0';
			while (txt[i]!='\\') 
				txt[i--] = '\0';
			path->Text = gcnew String(txt);
		}
		delete curDir;
		curDir = new FileInfo();
		if (curFS == 1) // ntfs
		{
			ntfsOpenDir(msclr::interop::marshal_as<std::string>(path->Text));
		}
		else //fat32
		{
			if (p->cluster == 0) p->cluster = curBoot->rootFirstClust; //to root
			rootCurClust = p->cluster;
			readWholeRoot(curHddName, p->cluster, curAbsFirSec);
		}
		if (curDir != nullptr)
		{
			outCurDir(nullptr);
		}
	}
}
private: System::Void btShowEmpty_Click(System::Object^  sender, System::EventArgs^  e) {
	_NumbersChain* ind = new _NumbersChain;
	findEmptyDirs(ind);
	outCurDir(ind);
	delete ind;
}
};
}