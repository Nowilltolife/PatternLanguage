#pragma once

#include <span>
#include <vector>

#include <wolv/io/buffered_reader.hpp>
#include <pl/core/vm.hpp>

namespace pl::hlp {

    struct ReaderData {
        core::VirtualMachine *vm;
        u64 sectionId;
    };

    inline void evaluatorReaderFunction(ReaderData *data, void *buffer, u64 address, size_t size) {
        data->vm->readData(address, buffer, size, data->sectionId);
    }

    class MemoryReader : public wolv::io::BufferedReader<ReaderData, evaluatorReaderFunction> {
    public:
        using BufferedReader::BufferedReader;

        MemoryReader(core::VirtualMachine *vm, u64 sectionId, size_t bufferSize = 0x100000) : BufferedReader(&this->m_readerData, vm->getDataSize(), bufferSize) {
            this->m_readerData.vm = vm;
            this->m_readerData.sectionId = sectionId;
        }

    private:
        ReaderData m_readerData;
    };

}