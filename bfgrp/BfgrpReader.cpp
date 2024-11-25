//
// Created by cookieso on 01.07.24.
//

#include <optional>
#include <iostream>
#include "BfgrpReader.h"

BfgrpReader::BfgrpReader(uint8_t *data, size_t size) : m_Data(data), m_Size(size) {
    this->m_Header = reinterpret_cast<BfgrpHeader *>(data);

    for (int i = 0; i < this->m_Header->sectionNum; ++i) {
        auto section = this->m_Header->sectionInfo[i];
        switch (section.flag) {
            case 0x7800:
                m_Info = section;
                break;
            case 0x7801:
                m_File = section;
                break;
            case 0x7802:
                m_InfoEx = section;
                break;
            default:
                std::cerr << "Unsupported section with flag " << section.flag << std::endl;
                exit(1);
        }
    }
}

void BfgrpReader::readGroupItemLocInfo(int index) {

}
