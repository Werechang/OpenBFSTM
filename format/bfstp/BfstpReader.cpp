//
// Created by cookieso on 07.12.24.
//

#include "BfstpReader.h"

BfstpReader::BfstpReader(const MemoryResource &resource) : m_Stream(resource) {

}

bool BfstpReader::readHeader() {
    return false;
}
