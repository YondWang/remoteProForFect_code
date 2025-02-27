#include "Command.h"
#include "pch.h"

CCommand::CCommand() : threadId(0)
{
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1, &CCommand::MakeDriverInfo},
		{2, &CCommand::MakeDirectoryInfo},
		{3, &CCommand::RunFile},
		{4, &CCommand::DownloadFile},
		{5, &CCommand::mouseEvent},
		{6, &CCommand::SendScreen},
		{7, &CCommand::LockMachine},
		{8, &CCommand::UnlockMachine},
		{9, &CCommand::DeleteLocalFile},
		{2001, &CCommand::TestConnect},
		{-1, NULL}
	};
	for (int i = 0; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCommend(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	return (this->*it->second)(lstPacket, inPacket);
}
