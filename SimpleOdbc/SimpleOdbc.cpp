﻿#include <stdio.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include "../ProudNet/ProudNet/include/BasicTypes.h"
#include "../ProudNet/ProudNet/include/OdbcWrap.h"

using namespace Proud;
using namespace std;


/*
for linux : To compile on Linux, you need to include the following options:
-lProudNet -lodbc -lpthread
*/
/*
linux 사용 시 : linux에서 컴파일 시, 아래 옵션을 포함해야 한다.
				-lProudNet -lodbc -lpthread
*/

// NOTE::The COdbcWarning occurs when a MsSql option(ex. database(schema) or language) is changed.
// 
// MsSql option이 변경되면 COdbcWarning이 발생한다. 
void MsSqlTest()
{
	_tprintf_s(_PNT("- MSSQL ODBC API Test Start.\n"));

	String connectionStr = "Driver={ODBC Driver 17 for SQL Server};Server=localhost;Trusted_Connection=yes;";

	COdbcConnection conn;
	COdbcCommand cmd;
	COdbcRecordset record;
	COdbcWarnings warnings;

	int result;

	// Connection Open/Close Test
	//
	// Connection 연결 테스트
	try
	{
		_tprintf_s(_PNT("-- Connection Open/Close Test Start\n"));

		conn.Open(connectionStr, &warnings);

		for (int i = 0; i < warnings.GetCount(); i++)
		{
			printf("INFO[1]::%d = %s\n", warnings[i].GetSqlErrorCode(), warnings[i].what());
		}
		conn.Close();

		conn.Open(connectionStr, &warnings);

		_tprintf_s(_PNT("-- Connection Open/Close Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Direct Execute Test
	//
	// 직접 쿼리를 실행하는 테스트
	try
	{
		_tprintf_s(_PNT("-- Direct Execute Test Start\n"));

		String query;

		// Create Database
		conn.Execute(_PNT("IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = 'proudnetodbctest') DROP DATABASE proudnetodbctest"));
		conn.Execute(_PNT("CREATE DATABASE proudnetodbctest"));
		conn.Execute(_PNT("USE proudnetodbctest"), &warnings);
		for (int i = 0; i < warnings.GetCount(); i++)
		{
			printf("INFO[4]::%d = %s\n", warnings[i].GetSqlErrorCode(), warnings[i].what());
		}
		conn.Execute(_PNT("CREATE TABLE test(id int primary key, name nvarchar(30))"));
		conn.Execute(_PNT("CREATE PROCEDURE insertTest @pId int, @pName nvarchar(30) AS BEGIN INSERT INTO test(id, name) VALUES(@pId, @pName); END"));
		conn.Execute(_PNT("CREATE PROCEDURE outputTest @pId int, @outdata nvarchar(30) OUTPUT AS BEGIN SELECT @outdata = name FROM test WHERE id = @pId; return 100; END"));

		query = _PNT("DELETE FROM test");
		result = conn.Execute(query);
		printf("%ws = %d [0]\n", query.GetString(), result);

		query = _PNT("INSERT INTO test(id) VALUES(1)");
		result = conn.Execute(query);
		printf("%ws = %d [1]\n", query.GetString(), result);

		query = _PNT("INSERT INTO test(id) VALUES(2)");
		result = conn.Execute(query);
		printf("%ws = %d [1]\n", query.GetString(), result);

		query = _PNT("INSERT INTO test(id) VALUES(3)");
		result = conn.Execute(query);
		printf("%ws = %d [1]\n", query.GetString(), result);

		_tprintf_s(_PNT("-- Direct Execute Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Transaction Test
	try
	{
		_tprintf_s(_PNT("-- Transaction Test Start\n"));

		String query;

		query = _PNT("SELECT * FROM test");
		result = conn.Execute(query);
		printf("%ws = %d [3]\n", query.GetString(), result);

		conn.BeginTrans();

		_tprintf_s(_PNT("BeginTrans\n"));
		query = _PNT("INSERT INTO test(id) VALUES(4)");

		result = conn.Execute(query);
		printf("%ws = %d [1]\n", query.GetString(), result);

		query = _PNT("DELETE FROM test");
		result = conn.Execute(query);
		printf("%ws = %d [4]\n", query.GetString(), result);

		conn.RollbackTrans();
		_tprintf_s(_PNT("RollbackTrans\n"));

		conn.BeginTrans();
		_tprintf_s(_PNT("BeginTrans\n"));

		query = _PNT("INSERT INTO test(id) VALUES(4)");
		result = conn.Execute(query);
		printf("%ws = %d [1]\n", query.GetString(), result);

		conn.CommitTrans();
		_tprintf_s(_PNT("CommitTrans\n"));

		query = _PNT("SELECT * FROM test");
		result = conn.Execute(query);
		printf("%ws = %d [4]\n", query.GetString(), result);

		_tprintf_s(_PNT("-- Transaction Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Query Prepare Execute Test
	try
	{
		_tprintf_s(_PNT("-- Query Prepare Execute Test Start\n"));

		int id;
		String name;

		cmd.Prepare(conn, _PNT("INSERT INTO test(id, name) VALUES(?, ?)"));
		cmd.AppendInputParameter(&id);
		cmd.AppendInputParameter(&name);

		id = 5;
		name = _PNT("Nettention");
		result = cmd.Execute();
		_tprintf_s(_PNT("INSERT INTO test(id, name) VALUES(5, 'Nettention') = %d [1]\n"), result);

		id = 6;
		name = _PNT("ProudNet");
		result = cmd.Execute();
		_tprintf_s(_PNT("INSERT INTO test(id, name) VALUES(6, 'ProudNet') = %d [1]\n"), result);

		/** NOTE::Input NULL Data. **/
		id = 7;
		cmd.SetNullParameter(2, true);
		result = cmd.Execute();
		cmd.SetNullParameter(2, false);
		_tprintf_s(_PNT("INSERT INTO test(id, name) VALUES(7, NULL) = %d [1]\n"), result);

		_tprintf_s(_PNT("-- Query Prepare Execute Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Stored Procedure Prepare Execute Test
	try
	{
		_tprintf_s(_PNT("-- Stored Procedure Prepare Execute Test Start\n"));

		int ret;
		int id;
		String name;

		cmd.Prepare(conn, _PNT("{call insertTest(?,?)}"));
		cmd.AppendInputParameter(&id);
		cmd.AppendInputParameter(&name);

		id = 8;
		name = _PNT("SPTest");
		result = cmd.Execute();
		_tprintf_s(_PNT("{call insertTest(8, 'SPTest')} = %d [1]\n"), result);

		id = 9;
		cmd.SetNullParameter(2, true);
		result = cmd.Execute();
		cmd.SetNullParameter(2, false);
		_tprintf_s(_PNT("{call insertTest(9, NULL)} = %d [1]\n"), result);

		cmd.Prepare(conn, _PNT("{? = call outputTest(?,?)}"));
		cmd.AppendOutputParameter(&ret);
		cmd.AppendInputParameter(&id);
		cmd.AppendOutputParameter(&name);

		id = 5;
		name.GetBuffer(100);
		result = cmd.Execute();
		name.ReleaseBuffer();
		_tprintf_s(_PNT("{? = call outputTest(5, &name)} = %d [1], %d = [100], %ws = [Nettention]\n"), result, ret, name.GetString());

		_tprintf_s(_PNT("-- Stored Procedure Prepare Execute Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Recordset Test
	try
	{
		_tprintf_s(_PNT("-- Recordset Test Start\n"));

		int id;
		String name;

		result = conn.Execute(record, _PNT("SELECT * FROM test"));
		_tprintf_s(_PNT("SELECT * FROM test = %d [9]\n"), result);

		while (record.MoveNext())
		{
			id = record.GetFieldValue(_PNT("id"));
			name = record.GetFieldValue(_PNT("name"));
			_tprintf_s(_PNT("id = %d, name = %ws\n"), id, name.GetString());
		}

		// Drop Database
		conn.Execute(_PNT("USE master"), &warnings);
		for (int i = 0; i < warnings.GetCount(); i++)
		{
			printf("INFO[5]::%d = %s\n", warnings[i].GetSqlErrorCode(), warnings[i].what());
		}
		conn.Execute(_PNT("DROP DATABASE proudnetodbctest"));

		_tprintf_s(_PNT("-- Recordset Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	_tprintf_s(_PNT("- MSSQL ODBC API Test End.\n"));
}

void MySqlTest()
{
	_tprintf_s(_PNT("- MYSQL ODBC API Test Start.\n"));

	String connectionStr = "Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;Uid=proud;Pwd=proudnet123;Option=3;";

	COdbcConnection conn;
	COdbcCommand cmd;
	COdbcRecordset record;

	int result;

	// Connection Open/Close Test
	try
	{
		_tprintf_s(_PNT("-- Connection Open/Close Test Start\n"));

		conn.Open(connectionStr);
		conn.Close();

		conn.Open(connectionStr);

		_tprintf_s(_PNT("-- Connection Open/Close Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Direct Execute Test
	try
	{
		_tprintf_s(_PNT("-- Direct Execute Test Start\n"));

		String query;

		// Create Database
		conn.Execute(_PNT("DROP DATABASE IF EXISTS proudnetodbctest"));
		conn.Execute(_PNT("CREATE DATABASE proudnetodbctest"));
		conn.Execute(_PNT("USE proudnetodbctest"));
		conn.Execute(_PNT("CREATE TABLE test(id int primary key, name varchar(30))"));
		conn.Execute(_PNT("CREATE PROCEDURE insertTest(pId int, pName varchar(30)) BEGIN INSERT INTO test(id, name) VALUES(pId, pName); END"));
		conn.Execute(_PNT("CREATE PROCEDURE outputTest(pId int, OUT outdata varchar(30)) BEGIN SELECT name INTO outdata FROM test WHERE id = pId; END"));

		query = _PNT("DELETE FROM test");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [0]\n"), query.GetString(), result);
		query = _PNT("INSERT INTO test(id) VALUES(1)");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [1]\n"), query.GetString(), result);
		query = _PNT("INSERT INTO test(id) VALUES(2)");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [1]\n"), query.GetString(), result);
		query = _PNT("INSERT INTO test(id) VALUES(3)");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [1]\n"), query.GetString(), result);

		_tprintf_s(_PNT("-- Direct Execute Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Transaction Test
	try
	{
		_tprintf_s(_PNT("-- Transaction Test Start\n"));

		String query;

		query = _PNT("SELECT * FROM test");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [3]\n"), query.GetString(), result);

		conn.BeginTrans();
		_tprintf_s(_PNT("BeginTrans\n"));
		query = _PNT("INSERT INTO test(id) VALUES(4)");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [1]\n"), query.GetString(), result);
		query = _PNT("DELETE FROM test");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [4]\n"), query.GetString(), result);
		conn.RollbackTrans();
		_tprintf_s(_PNT("RollbackTrans\n"));

		conn.BeginTrans();
		_tprintf_s(_PNT("BeginTrans\n"));
		query = _PNT("INSERT INTO test(id) VALUES(4)");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [1]\n"), query.GetString(), result);
		conn.CommitTrans();
		_tprintf_s(_PNT("CommitTrans\n"));

		query = _PNT("SELECT * FROM test");
		result = conn.Execute(query);
		_tprintf_s(_PNT("%ws = %d [4]\n"), query.GetString(), result);

		_tprintf_s(_PNT("-- Transaction Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Query Prepare Execute Test
	try
	{
		_tprintf_s(_PNT("-- Query Prepare Execute Test Start\n"));

		int id;
		String name;

		cmd.Prepare(conn, _PNT("INSERT INTO test(id, name) VALUES(?, ?)"));
		cmd.AppendInputParameter(&id);
		cmd.AppendInputParameter(&name);

		id = 5;
		name = _PNT("Nettention");
		result = cmd.Execute();
		_tprintf_s(_PNT("INSERT INTO test(id, name) VALUES(5, 'Nettention') = %d [1]\n"), result);

		id = 6;
		name = _PNT("ProudNet");
		result = cmd.Execute();
		_tprintf_s(_PNT("INSERT INTO test(id, name) VALUES(6, 'ProudNet') = %d [1]\n"), result);

		id = 7;
		cmd.SetNullParameter(2, true);
		result = cmd.Execute();
		cmd.SetNullParameter(2, false);
		_tprintf_s(_PNT("INSERT INTO test(id, name) VALUES(7, NULL) = %d [1]\n"), result);

		_tprintf_s(_PNT("-- Query Prepare Execute Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Stored Procedure Prepare Execute Test
	try
	{
		_tprintf_s(_PNT("-- Stored Procedure Prepare Execute Test Start\n"));

		int id;
		String name;

		cmd.Prepare(conn, _PNT("call insertTest(?,?)"));
		cmd.AppendInputParameter(&id);
		cmd.AppendInputParameter(&name);

		id = 8;
		name = _PNT("SPTest");
		result = cmd.Execute();
		_tprintf_s(_PNT("call insertTest(8, 'SPTest') = %d [1]\n"), result);

		id = 9;
		cmd.SetNullParameter(2, true);
		result = cmd.Execute();
		cmd.SetNullParameter(2, false);
		_tprintf_s(_PNT("call insertTest(9, NULL) = %d [1]\n"), result);

		cmd.Prepare(conn, _PNT("call outputTest(?,?)"));
		cmd.AppendInputParameter(&id);
		cmd.AppendOutputParameter(&name);

		id = 5;
		name.GetBuffer(100);
		result = cmd.Execute();
		name.ReleaseBuffer();
		_tprintf_s(_PNT("call outputTest(5, &name) = %d [-1], %ws = [Nettention]\n"), result, name.GetString());

		_tprintf_s(_PNT("-- Stored Procedure Prepare Execute Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	// Recordset Test
	try
	{
		_tprintf_s(_PNT("-- Recordset Test Start\n"));

		int id;
		String name;

		result = conn.Execute(record, _PNT("SELECT * FROM test"));
		_tprintf_s(_PNT("SELECT * FROM test = %d [9]\n"), result);

		while (record.MoveNext())
		{
			id = record.GetFieldValue(_PNT("id"));
			name = record.GetFieldValue(_PNT("name"));
			_tprintf_s(_PNT("id = %d, name = %ws\n"), id, name.GetString());
		}

		// Drop Database
		conn.Execute(_PNT("DROP DATABASE proudnetodbctest"));

		_tprintf_s(_PNT("-- Recordset Test End\n"));
	}
	catch (COdbcException &e)
	{
		printf("Error::%d = %s\n", e.GetSqlErrorCode(), e.what());
		return;
	}

	_tprintf_s(_PNT("- MYSQL ODBC API Test End.\n"));
}

int main()
{
	char inMenu = 'n';
	_tprintf_s(_PNT("START...\n"));

	cout << "Will you test ODBC for Ms-SQL (or MySQL)? (y or n): ";

	std::cin >> inMenu;

	switch (inMenu)
	{
	case 'n':
		MySqlTest();
		break;
	default:
		MsSqlTest();
		break;
	}

	_tprintf_s(_PNT("...END\n"));
	system("pause");
	return 0;
}
