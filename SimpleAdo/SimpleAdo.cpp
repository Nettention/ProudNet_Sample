// SimpleAdo.cpp 2009.10.20 create by ulelio
//

#include "stdafx.h"
#include "../../include/AdoWrap.h"
#include "../../include/ProudNetServer.h"
#include "../../include/CoInit.h"

#define MYSQL

using namespace Proud;

// 테스트 용 테이블은 프로젝트 경로에 있는 .sql 파일을 참고하세요.

// Connect to DB.
// 
// DB에 연결한다.
void DBConnect( CAdoConnection& conn )
{
	try
	{
#ifdef MYSQL
		conn.Open(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;Database=proudado-test;User ID=proud;Password=proudnet123;Option=3;", MySql);
#endif
#ifdef MSSQL
		conn.Open(L"Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=ProudAdo-Test;Trusted_Connection=yes;");
#endif
		wprintf( L"Succeed to connect DB!!\n");
	}
	catch(AdoException &e)
	{
		wprintf(L"DB Exception : %s\n", Proud::StringA2W(e.what()).GetString());
	}

}

// Print FieldName of UserData.
//
// UserData 의 FieldName을 출력한다.
void PrintFieldNameToUserTable(CAdoRecordset& rec)
{
	wprintf(L"===========================================================\n");

	wprintf(L"FieldName1 : %s, FieldName2 : %s, FieldName3 : %s\n",
		((String)rec.GetFieldNames(0)).GetString(),
		((String)rec.GetFieldNames(1)).GetString(),
		((String)rec.GetFieldNames(2)).GetString());

}

// Print all Field value of UserData.
//
// UserData의 모든 Field의 값을 출력한다.
void PrintFieldValueToUserTable(CAdoRecordset& rec)
{
#ifdef MYSQL
	String strID, strPassword, strCountry;
#endif
#ifdef MSSQL
	String strID, strPassword, strCountry;
#endif

	wprintf(L"===========================================================\n");

	while (rec.IsEOF() == false)
	{
#ifdef MYSQL
		strID = rec.FieldValues[L"UserID"];
		strPassword = rec.FieldValues[L"Password"];
		strCountry = rec.FieldValues[L"Country"];
#endif
#ifdef MSSQL
		strID = ((String)rec.FieldValues[L"UserID"]).GetString();
		strPassword = ((String)rec.FieldValues[L"Password"]).GetString();
		strCountry = ((String)rec.FieldValues[L"Country"]).GetString();
#endif
		wprintf(L"UserID : %s, Password : %s, Country : %s\n", strID.GetString(), strPassword.GetString(), strCountry.GetString());

		// Move a cursor to next line.
		//
		// 커서를 다음행으로 이동시킨다.
		rec.MoveNext();
	}
}

// UserData를 읽어온다.
void SelectUserData(CAdoConnection& conn, CAdoRecordset& rec) {
	// Select UserData
	//
	// UserData을 Select 한다.
	try
	{
		rec.Open(conn, OpenForReadWrite, L"select * from UserData");
	}
	catch (AdoException& e)
	{
		wprintf(L"DB Exception : %s\n", Proud::StringA2W(e.what()).GetString());
	}

	// Print FieldName
	//
	// FieldName 출력
	PrintFieldNameToUserTable(rec);

	// Print FieldValue
	// 
	// FieldValue 출력
	PrintFieldValueToUserTable(rec);

	rec.Close();
}

// Insert value to UserData. ( Example of query excution )
// 
// UserData 에 값을 삽입한다. ( 쿼리실행 예 )
void InsertUserTable( CAdoConnection& conn, const wchar_t* szUserID, const wchar_t* szPassword, int nCountry )
{
	String query = L"insert into UserData (UserID, Password, Country) VALUES(%s, %s, %d)";
	try
	{
		// Start transaction
		//  
		// 트랜잭션 시작 
		conn.BeginTrans();

		wprintf( L"Execute Query : insert into UserData (UserID, Password, Country ) VALUES(%s, %s, %d)....\n", szUserID, szPassword, nCountry );

		// Excute query.
		// 
		// 쿼리를 실행한다.
		conn.Execute(String::NewFormat(L"insert into UserData (UserID, Password, Country) VALUES(%s, %s, %d)", szUserID, szPassword, nCountry));

		wprintf(( L"Succeed to execute Query~!!\n") );

		// transaction Commit
		//
		// 트랜잭션 Commit
		conn.CommitTrans();
	}
	catch (AdoException &e)
	{
		wprintf( L"DB Exception : %s\n", Proud::StringA2W(e.what()).GetString());
	}
}

// Excute AddUserData stored procedure.
//
// AddUserData stored procedure를 실행해보자.
void Excute_AddUserData( CAdoConnection& conn, CAdoRecordset& rec, const wchar_t* szUserID, const wchar_t* szPassword, int nCountry )
{
	CAdoCommand cmd;
	cmd.Prepare(conn, L"AddUserData");

#ifdef MYSQL
	// MSSQL과 달리 AppendParameter을 호출해야만 한다.
	// Need to call AppendParameter instead of MSSQL.
	cmd.AppendParameter(L"InUserID", ADODB::adVarWChar, ADODB::adParamInput, szUserID, wcslen(szUserID));
	cmd.AppendParameter(L"InPassword", ADODB::adVarWChar, ADODB::adParamInput, szPassword, wcslen(szPassword));
	cmd.AppendParameter(L"InCountry", ADODB::adInteger, ADODB::adParamInput, nCountry);

	cmd.Execute();

	// MySql은 리턴을 지원하지 않음.output파라메터 또한 지원하지 않음.
	// MySql does not support return. Also does not support output parameter too.
	try
	{
		if (!rec.IsOpened())
		{
			// 리턴받은  recordset 객체를 먼저 열어야 접근가능
			// Has to open recordset object to access it
			rec.Open(conn, OpenForRead, L"select * from UserData");
		}
	}
	catch (AdoException& e)
	{
		wprintf(L"DB Exception : %s\n", StringA2W(e.what()).GetString());
	}
#endif
#ifdef MSSQL
	StringA szUserIDA = (LPSTR)CW2A(szUserID);
	cmd.Parameters[1] = szUserIDA.GetString();
	cmd.Parameters[2] = szPassword;
	cmd.Parameters[3] = nCountry;

	cmd.Execute(rec);
	long ret = cmd.Parameters[0];

	// Return -1 when procedure is failed.
	//
	// 프로시져가 실패하면 -1을 리턴함.
	if (ret < 0)
	{
		wprintf(L"Stored Procedure is failed!!\n");
	}
	else
	{
		try
		{
			if (!rec.IsOpened())
			{
				rec.Open(conn, OpenForRead, L"select * from UserData"); // 리턴받은  recordset 객체를 먼저 열어야 접근가능
			}
		}
		catch (AdoException& e)
		{
			wprintf(L"DB Exception : %s\n", Proud::StringA2W(e.what()).GetString());
		}
	}
#endif

	// Print again
	//
	// 다시 출력해보자
	PrintFieldValueToUserTable(rec);

	rec.Close_NoThrow();
}

int _tmain(int argc, _TCHAR* argv[])
{
	// 다국어 출력을 위해 locale를 설정합니다.
	setlocale(LC_ALL, "");

	// Coinitialize has to called. It is convenience if you use Proud::CCoInitializer.
	//
	// Coinitialize가 호출되있어야한다. Proud::CCoInitializer를 사용하면 편리하다.
	CCoInitializer coi;

	// Create ADO connect object.
	// 
	// ADO 연결객체를 생성한다.
	CAdoConnection conn;

	DBConnect(conn);

	// TestTimeField(conn);

	wchar_t szUserID[20] = L"'테스트'";
	wchar_t szPassword[20] = L"'1234'";
	int nCountry = 11;

	InsertUserTable(conn, szUserID, szPassword, nCountry);
	
	// Create object to get recordset
	// 
	// recordset을 얻어오기 위한 객체 생성
	CAdoRecordset rec;
	
	SelectUserData(conn, rec);

	wcscpy_s( szUserID, L"ulelio1" );
	wcscpy_s( szPassword, L"1234a" );
	nCountry = 11;

	// Execute procedure
	//	
	// 프로시져 실행
	Excute_AddUserData(conn, rec, szUserID, szPassword, nCountry);

	system("PAUSE");

	return 0;
}

