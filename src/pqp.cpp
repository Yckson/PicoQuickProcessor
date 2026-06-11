#include <bits/stdc++.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <sys/ucontext.h>

#define MAX_CODE_BLOCK_SIZE 4 * 1024

#define LINE_HEADER(c, addr) \
    std::setfill(' ') << std::setw(8) << std::dec \
    << (c) << ':' << std::setfill('0') << std::setw(4) << std::hex << (addr) << ':' << std::dec

#define PRINT_MOVI(c, addr, rx, i) \
    "mov_r" << (int) (rx) << ',' << std::hex << std::setfill('0') << std::setw(8) << (int) (i)

#define PRINT_MOVR(c, addr, rx, ry) \
    "mov_r" << (int) (rx) << ",r" << (int) (ry)

#define PRINT_MOVRM(c, addr, rx, ry) \
    "mov_r" << (int) (rx) << ",[r" << (int) (ry) << ']'

#define PRINT_MOVMR(c, addr, rx, ry) \
    "mov_[r" << (int) (rx) << "],r" << (int) (ry)

#define PRINT_CMP(c, addr, rx, ry) \
    "cmp_r" << (int) (rx) << "<=>r" << (int) (ry)

#define PRINT_JMP(c, addr, jmp) \
    "jmp_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JG(c, addr, jmp) \
    "jg_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JL(c, addr, jmp) \
    "jl_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JE(c, addr, jmp) \
    "je_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_ADD(c, addr, rx, ry) \
     "add_r" << (int) (rx) << "+=r" << (int) (ry)

#define PRINT_SUB(c, addr, rx, ry) \
     "sub_r" << (int) (rx) << "-=r" << (int) (ry)

#define PRINT_AND(c, addr, rx, ry) \
     "and_r" << (int) (rx) << "&=r" << (int) (ry)

#define PRINT_OR(c, addr, rx, ry) \
     "or_r" << (int) (rx) << "|=r" << (int) (ry)

#define PRINT_XOR(c, addr, rx, ry) \
     "xor_r" << (int) (rx) << "^=r" << (int) (ry)

#define PRINT_SAL(c, addr, rx, i) \
     "sal_r" << (int) (rx) << "<<=" << (int) (i)

#define PRINT_SAR(c, addr, rx, i) \
     "sar_r" << (int) (rx) << ">>=" << (int) (i)

#define PRINT_REG(rx)\
    'r' << std::dec << (rx) << '=' << std::hex << std::setfill('0') << std::setw(8)

namespace PQP {

    enum OPCODE : uint8_t {
        MOVI,               // MOV Rx   ,   i16      SIGN-EXTENDED
        MOVR,               // MOV Rx   ,   Ry
        MOVRM,              // MOV Rx   ,   [Ry]
        MOVMR,              // MOV [Rx] ,   [Ry]
        CMP,                // CMP Rx   ,   Ry
        JMP,                // JMP          i16      SIGN-EXTENDED + 4
        JG,                 // JG           i16      SIGN-EXTENDED + 4
        JL,                 // JL           i16      SIGN-EXTENDED + 4
        JE,                 // JE           i16      SIGN-EXTENDED + 4
        ADD,                // ADD Rx   ,   Ry
        SUB,                // SUB Rx   ,   Ry
        AND,                // AND Rx   ,   Ry
        OR,                 // OR  Rx   ,   Ry
        XOR,                // XOR Rx   ,   Ry
        SAL,                // Rx           i5       
        SAR                 // Rx           i5       
    };

    struct INSTRUCTION {
        OPCODE opcode;
        uint8_t Rx;
        uint8_t Ry;
        int16_t i16;
        uint8_t i5;

        INSTRUCTION (uint8_t raw_instruction [4]){
            this->opcode = (OPCODE) raw_instruction[0];
            this->Rx  = raw_instruction[1] >> 4;
            this->Ry  = raw_instruction[1] & 0x0F;
            this->i16 = (int16_t) (*((uint16_t*) &raw_instruction[2]));
            this->i5  = raw_instruction[3] & 0x0F;
        }
    };

    static inline bool get_raw_instruction (std::ifstream& input, uint8_t raw_instruction [4]){
        for (int i = 0; i < 4; i++){
            uint32_t n;
            if (!(input >> std::hex >> n)) return false;
            raw_instruction[i] = (uint8_t) n;
        }
        return true;
    }

    uint8_t* pqp_mem;
    uint32_t pqp_addr_counter [256];
    int32_t pqp_regs  [16] ;
    uint8_t pqp_flags =   0;
    uint32_t PC       =   0;

    std::map<uint32_t, std::pair<uint32_t, std::string>> mapped_instructions;

    void load_memory (std::ifstream& input){
        uint8_t raw_instruction [4];
        
        
        pqp_mem = (uint8_t*) mmap((void*) 0x10000000, 
                                  256, 
                                  PROT_READ | PROT_WRITE, 
                                  MAP_32BIT | MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED_NOREPLACE, 
                                  -1, 
                                  0
                                );

        if (pqp_mem == MAP_FAILED){
            std::cout << "Falha no mmap!" << std::endl;
            exit(0);
        }
        //memset(pqp_mem, 0, 256);

        std::cout << "Loading PQP memory (256 Bytes)..." << std::endl;
        int i;
        for (i = 0; i < 256; i+=4){
            if (!get_raw_instruction(input, raw_instruction)) break;
            pqp_mem[i]   = raw_instruction[0];
            pqp_mem[i+1] = raw_instruction[1];
            pqp_mem[i+2] = raw_instruction[2];
            pqp_mem[i+3] = raw_instruction[3];
        }

        while (i++ < 256){
            pqp_mem[i] = 0;
        }

        std::cout << "Memory loaded! Address: 0x" << std::hex << (uint64_t) pqp_mem << std::endl;
    }
}

namespace JIT {

    enum X86_REGS {
        EAX, ECX, EDX,  EBX,  ESP,  EBP,  ESI,  EDI,
        R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D
    };

    namespace REG_TYPE {
        enum REG_TYPE{
            R_OLD = 7,
            R_NEW = 8
        };
    }

    struct sigaction actionINT;
    struct sigaction actionSEGFAUT;
    stack_t alstack;
    uint8_t* executable_memory;
    size_t executable_memory_index;
    uint8_t raw_code[MAX_CODE_BLOCK_SIZE];
    size_t next_counter_index = 0;

    uint32_t* block_counters_arena;
    uint32_t* pc_constants_arena;

    struct BlockMeta {
        uint32_t guest_pc_start;     
        uint32_t instruction_count;  
        uint32_t* counter_ptr;
    };

    std::vector<BlockMeta> block_metadata_table;
    std::unordered_map<uint32_t, uint8_t*> block_map;

    template <PQP::OPCODE T>
    size_t emmit_MOV (PQP::INSTRUCTION& instruction, uint8_t* gen_code){
        size_t pos = 0;

        switch (T) {
            case PQP::OPCODE::MOVI: {
                if (instruction.Rx >= REG_TYPE::R_NEW) gen_code[pos++] = 0x41; //REX.B
                gen_code[pos++] = 0xB8 + (instruction.Rx & 0x7);
                *((int32_t*) &gen_code[pos]) = (int32_t) instruction.i16;
                pos += sizeof(int32_t);
                break;
            }

            case PQP::OPCODE::MOVR: {
                if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
                    gen_code[pos++] = 0x41; // REX.B
                }
                else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x44; // REX.R
                }
                else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x45; // REX.R | REX.B
                }

                gen_code[pos++] = 0x89;
                gen_code[pos++] = 0xC0 + (instruction.Rx & 0x7) + ((instruction.Ry & 0x7) << 3);
                break;
            }

            case PQP::OPCODE::MOVRM: {
                gen_code[pos++] = 0x67;

                if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
                    gen_code[pos++] = 0x44; //REX.R
                }
                else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x41; //REX.B
                }
                else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x45; //REX.R | REX.B
                }

                gen_code[pos++] = 0x8B;
                gen_code[pos++] = 0x80 + (instruction.Ry & 0x7) + ((instruction.Rx & 0x7) << 3);
                *((int32_t*) &gen_code[pos]) = (uint64_t) PQP::pqp_mem;
                pos += 4;
                break;
            }
            case PQP::OPCODE::MOVMR: {
                gen_code[pos++] = 0x67;

                if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
                    gen_code[pos++] = 0x41;
                }
                else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x44;
                }
                else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x45;
                }

                gen_code[pos++] = 0x89;
                gen_code[pos++] = 0x80 + (instruction.Rx & 0x7) + ((instruction.Ry & 0x7) << 3);
                *((int32_t*) &gen_code[pos]) = (uint64_t) PQP::pqp_mem;
                pos += 4;
                break;
            }
        }
        return pos;
    }

    // Emissor unificado para operações lógicas e aritméticas (ADD, SUB, AND, OR, XOR, CMP)
    template <PQP::OPCODE OP>
    size_t emmit_BINARY_OP (PQP::INSTRUCTION& instruction, uint8_t* gen_code){
        size_t pos = 0;

        // Regra do REX para OP r/m32, r32 (Onde Rx é r/m32 e Ry é r32)
        if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
            gen_code[pos++] = 0x41; // REX.B (Rx estendido)
        }
        else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
            gen_code[pos++] = 0x44; // REX.R (Ry estendido)
        }
        else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
            gen_code[pos++] = 0x45; // REX.R | REX.B (Ambos)
        }

        // Emitir o Opcode correto com base na instrução
        switch (OP) {
            case PQP::ADD: gen_code[pos++] = 0x01; break;
            case PQP::SUB: gen_code[pos++] = 0x29; break;
            case PQP::AND: gen_code[pos++] = 0x21; break;
            case PQP::OR:  gen_code[pos++] = 0x09; break;
            case PQP::XOR: gen_code[pos++] = 0x31; break;
            case PQP::CMP: gen_code[pos++] = 0x39; break;
            default: break;
        }

        // ModR/M: Mod = 11 (0xC0), Reg = Ry, R/M = Rx
        gen_code[pos++] = 0xC0 + (instruction.Rx & 0x7) + ((instruction.Ry & 0x7) << 3);

        return pos;
    }

    // Emissor unificado para operações de deslocamento de bits (SAL, SAR)
    template <PQP::OPCODE OP>
    size_t emmit_SHIFT_OP (PQP::INSTRUCTION& instruction, uint8_t* gen_code){
        size_t pos = 0;

        // O Rx é o destino e fica no campo R/M, logo controla o REX.B
        if (instruction.Rx >= REG_TYPE::R_NEW){
            gen_code[pos++] = 0x41; // REX.B
        }

        // Opcode base para shift com imediato
        gen_code[pos++] = 0xC1;

        // O campo Reg atua como extensão do opcode (SAL = 4, SAR = 7)
        if constexpr (OP == PQP::SAL) {
            gen_code[pos++] = 0xE0 + (instruction.Rx & 0x7); // 0xC0 + (4 << 3) = 0xE0
        } 
        else if constexpr (OP == PQP::SAR) {
            gen_code[pos++] = 0xF8 + (instruction.Rx & 0x7); // 0xC0 + (7 << 3) = 0xF8
        }

        // Valor imediato
        gen_code[pos++] = instruction.i5;

        return pos;
    }

    size_t emmit_raw_xorps_xmm0_xmm0 (uint8_t* gen_code){
        size_t pos = 0;
        //pxor xmm0, xmm0
        gen_code[pos++] = 0x0F;
        gen_code[pos++] = 0x57;
        gen_code[pos++] = 0xC0;

        return pos;
    }

    size_t emmit_raw_xorps_xmm1_xmm1 (uint8_t* gen_code){
        size_t pos = 0;
        // xorps xmm1, xmm1 -> Zera o xmm1 (Guest PC = 0)
        gen_code[pos++] = 0x0F;
        gen_code[pos++] = 0x57;
        gen_code[pos++] = 0xC9; 

        return pos;
    }

    size_t emmit_raw_xmm2_constant1 (uint8_t* gen_code, uint32_t constant1){
        size_t pos = 0;
        //movd xmm2, dword ptr [addr]

        gen_code[pos++] = 0x66;
        gen_code[pos++] = 0x0F;
        gen_code[pos++] = 0x6E;
        gen_code[pos++] = 0x14;
        gen_code[pos++] = 0x25;
        *((uint32_t*)(&gen_code[pos])) = constant1;
        pos += sizeof(uint32_t);

        return pos;
    }

    size_t emmit_raw_jmp_placeholder (uint8_t* gen_code){
        size_t pos = 0;
        gen_code[pos++] = 0xCC;
        uint32_t temp = 0x90909090;
        *((uint32_t*) (&gen_code[pos])) = temp;
        pos += 4;
        return pos;
    }


    size_t emmit_raw_blockchanning_jmp (uint8_t* gen_code, int32_t offset){
        size_t pos = 0;
        gen_code[pos++] = 0xE9;
        *((uint32_t*) (&gen_code[pos])) = (uint32_t) offset;
        pos += sizeof(uint32_t);
        return pos;
    }

    size_t emmit_raw_update_guest_pc(uint8_t* gen_code, uint32_t target_pc_address) {
        size_t pos = 0;

        // vmovd xmm1, dword ptr 
        gen_code[pos++] = 0x66; 
        gen_code[pos++] = 0x0F; 
        gen_code[pos++] = 0x6E;
        gen_code[pos++] = 0x0C; 
        gen_code[pos++] = 0x25; 
        
        *((uint32_t*)(&gen_code[pos])) = target_pc_address;
        pos += sizeof(uint32_t);

        return pos;
    }

    size_t emmit_raw_block_header (uint8_t* gen_code, uint32_t counter_addr){
        size_t pos = 0;

        // --- PARTE 1: Atualiza o Guest PC ---
        // movd xmm1, dword ptr [pc_address]
        // gen_code[pos++] = 0x66; gen_code[pos++] = 0x0F; gen_code[pos++] = 0x6E;
        // gen_code[pos++] = 0x0C; gen_code[pos++] = 0x25;
        // *((uint32_t*) (&gen_code[pos])) = pc_address;
        // pos += sizeof(uint32_t);

        // --- PARTE 2: Incrementa o Contador do Bloco ---
        // movd xmm0, dword ptr [counter_addr]
        gen_code[pos++] = 0x66; gen_code[pos++] = 0x0F; gen_code[pos++] = 0x6E;
        gen_code[pos++] = 0x04; gen_code[pos++] = 0x25;
        *((uint32_t*) (&gen_code[pos])) = counter_addr;
        pos += sizeof(uint32_t);

        // paddd xmm0, xmm2 (xmm2 tem o valor 1 globalmente)
        gen_code[pos++] = 0x66; gen_code[pos++] = 0x0F; gen_code[pos++] = 0xFE; 
        gen_code[pos++] = 0xC2;

        // movd dword ptr [counter_addr], xmm0
        gen_code[pos++] = 0x66; gen_code[pos++] = 0x0F; gen_code[pos++] = 0x7E;
        gen_code[pos++] = 0x04; gen_code[pos++] = 0x25;
        *((uint32_t*) (&gen_code[pos])) = counter_addr;
        pos += sizeof(uint32_t);

        return pos;
    }

    template <PQP::OPCODE OP>
    size_t emmit_JMP_COND_OP (PQP::INSTRUCTION& instruction, uint8_t* gen_code){
        size_t pos = 0;
        gen_code[pos++] = 0x0F; 
        switch (OP) {
            case PQP::OPCODE::JG: {
                gen_code[pos++] = 0x8F;
                break;
            }
            case PQP::OPCODE::JL: {
                gen_code[pos++] = 0x8C;
                break;
            }
            case PQP::OPCODE::JE: {
                gen_code[pos++] = 0x84;
                break;
            }
        }

        *((uint32_t*) &(gen_code[pos])) = 14; //SIZE OF FOOTER
        return pos + sizeof(uint32_t);


    }
    
    std::string to_string(uint8_t* gen_code, size_t size) {
        std::ostringstream s;
        for (size_t i = 0; i < size; i++){
            s << "0x" << std::hex << std::setfill('0') << std::setw(2)
            << (int) gen_code[i] << ' ';
        }
        return s.str();
    }

    void print_memory_dump(const uint8_t* addr, size_t size) {
        const size_t bytes_per_line = 32;

        // Configura o cout para imprimir em hexadecimal, com letras maiúsculas e preenchimento de zeros
        std::cout << std::uppercase << std::hex << std::setfill('0');

        for (size_t i = 0; i < size; i += bytes_per_line) {
            // [Opcional] Imprime o endereço base da linha para facilitar a localização
            std::cout << "0x" << std::setw(8) << reinterpret_cast<uintptr_t>(addr + i) << " | ";

            // Loop para imprimir os 32 bytes da linha atual
            for (size_t j = 0; j < bytes_per_line; ++j) {
                if (i + j < size) {
                    // Imprime o byte real da memória executável
                    std::cout << std::setw(2) << static_cast<int>(addr[i + j]) << " ";
                } else {
                    // Completa o alinhamento com "XX" se o tamanho acabar antes dos 32 bytes
                    std::cout << "XX ";
                }
            }
            std::cout << '\n'; // Quebra a linha após 32 colunas
        }
        std::cout << std::endl;
        // Restaura a formatação padrão do cout (decimal) para não afetar outros prints do seu emulador
        std::cout << std::dec << std::nouppercase;
    }

    uint32_t get_new_counter_address() {
        uint32_t* counter_ptr = &block_counters_arena[next_counter_index];
        next_counter_index++;
        
        return (uint32_t)(uintptr_t)counter_ptr;
    }

    uint32_t get_pc_const_address(uint32_t current_pc) {
        uint32_t* const_ptr = &pc_constants_arena[current_pc];
        
        return (uint32_t)(uintptr_t)const_ptr;
    }

    uint8_t* interpreter (uint32_t guest_PC){

        using namespace PQP;
        std::vector<uint8_t> code_block;
        uint32_t counter = 0;

        uint32_t counter_pointer = get_new_counter_address();
        
        

        size_t h_size = emmit_raw_block_header(raw_code, counter_pointer);
        code_block.insert(code_block.begin(), raw_code, raw_code + h_size);
        

        

        
        PC = guest_PC;
        std::cout << guest_PC << std::endl;
        bool out = false;
        
        while (PC < 256 && !out){
            
            counter++;
            
            INSTRUCTION i (&pqp_mem[PC]);
            
            switch (i.opcode) {
                case MOVI: {
                    size_t size = JIT::emmit_MOV<PQP::OPCODE::MOVI>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    }
                    else{
                        std::stringstream s;
                        s << PRINT_MOVI(++pqp_addr_counter[PC], PC, i.Rx, i.i16);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "MOV_I: " << i_text << std::endl;
                    }
                    
                    break;
                }
                case MOVR: {
                    size_t size = JIT::emmit_MOV<PQP::OPCODE::MOVR>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_MOVR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "MOV_R: " << i_text << std::endl;
                    }
                    break;
                }
                case MOVRM: {
                    size_t size = JIT::emmit_MOV<PQP::OPCODE::MOVRM>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_MOVRM(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "MOV_R_M: " << i_text << std::endl;
                    }
                    break;
                }
                case MOVMR: {
                    size_t size = JIT::emmit_MOV<PQP::OPCODE::MOVMR>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_MOVMR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "MOV_M_R: " << i_text << std::endl;
                    }
                    break;
                }
                case CMP: {
                    size_t size = JIT::emmit_BINARY_OP<PQP::CMP>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_CMP(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "CMP: " << i_text << std::endl;
                    }
                    break;
                }
                case JMP: {
                    uint32_t tmp = PC;
                    uint32_t target = tmp + (int32_t)i.i16 + 4;
                    uint32_t target_const_address = get_pc_const_address(target);

                    size_t size = emmit_raw_update_guest_pc(raw_code, target_const_address);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_jmp_placeholder(raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    
                    if (mapped_instructions.count(tmp)) {
                        mapped_instructions[tmp].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_JMP(++pqp_addr_counter[tmp], tmp, target);
                        mapped_instructions[tmp] = {1, s.str()};
                    }
                    std::cout << PRINT_JMP(0, tmp, target) << std::endl;
                    out = true;
                    break;
                }
                case JG: {
                    uint32_t tmp = PC;
                    uint32_t target = tmp + (int32_t)i.i16 + 4;

                    uint32_t target_const_address = get_pc_const_address(target);
                    uint32_t PC_const_address = get_pc_const_address(PC+4);

                    size_t size = emmit_JMP_COND_OP<PQP::OPCODE::JG>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_update_guest_pc(raw_code, PC_const_address);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_jmp_placeholder(raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    size = emmit_raw_update_guest_pc(raw_code, target_const_address);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_jmp_placeholder(raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);
                    
                    if (pqp_flags & 0b010) PC += (int32_t) i.i16;
                    if (mapped_instructions.count(tmp)) {
                        mapped_instructions[tmp].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_JG(++pqp_addr_counter[tmp], tmp, target);
                        mapped_instructions[tmp] = {1, s.str()};
                    }

                    
                    std::cout << PRINT_JG(0, tmp, target) << std::endl;
                    out = true;
                    break;
                }
                case JL: {
                    uint32_t tmp = PC;
                    uint32_t target = tmp + (int32_t)i.i16 + 4;

                    uint32_t target_const_address = get_pc_const_address(target);
                    uint32_t PC_const_address = get_pc_const_address(PC+4);

                    size_t size = emmit_JMP_COND_OP<PQP::OPCODE::JL>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_update_guest_pc(raw_code, PC_const_address);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_jmp_placeholder(raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    size = emmit_raw_update_guest_pc(raw_code, target_const_address);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_jmp_placeholder(raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);
                    if (pqp_flags & 0b001) PC += (int32_t) i.i16;
                    if (mapped_instructions.count(tmp)) {
                        mapped_instructions[tmp].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_JL(++pqp_addr_counter[tmp], tmp, target);
                        mapped_instructions[tmp] = {1, s.str()};
                    }
                    std::cout << PRINT_JL(0, tmp, target) << std::endl;
                    out = true;
                    break;
                }
                case JE: {
                    uint32_t tmp = PC;
                    uint32_t target = tmp + (int32_t)i.i16 + 4;

                    uint32_t target_const_address = get_pc_const_address(target);
                    uint32_t PC_const_address = get_pc_const_address(PC+4);

                    size_t size = emmit_JMP_COND_OP<PQP::OPCODE::JE>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_update_guest_pc(raw_code, PC_const_address);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_jmp_placeholder(raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    size = emmit_raw_update_guest_pc(raw_code, target_const_address);
                    code_block.insert(code_block.end(), raw_code, raw_code+size);

                    size = emmit_raw_jmp_placeholder(raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (pqp_flags & 0b100) PC += (int32_t) i.i16;
                    if (mapped_instructions.count(tmp)) {
                        mapped_instructions[tmp].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_JE(++pqp_addr_counter[tmp], tmp, target);
                        mapped_instructions[tmp] = {1, s.str()};
                    }
                    std::cout << PRINT_JE(0, tmp, target) << std::endl;
                    out = true;
        
                    
                    break;
                }
                case ADD: {
                    size_t size = JIT::emmit_BINARY_OP<PQP::ADD>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_ADD(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "ADD: " << i_text << std::endl;
                    }
                    break;
                }
                case SUB: {
                    size_t size = JIT::emmit_BINARY_OP<PQP::SUB>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_SUB(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "SUB: " << i_text << std::endl;
                    }
                    break;
                }
                case AND: {
                    size_t size = JIT::emmit_BINARY_OP<PQP::AND>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_AND(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "AND: " << i_text << std::endl;
                    }
                    break;
                }
                case OR: {
                    size_t size = JIT::emmit_BINARY_OP<PQP::OR>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_OR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "OR: " << i_text << std::endl;
                    }
                    break;
                }
                case XOR: {
                    size_t size = JIT::emmit_BINARY_OP<PQP::XOR>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_XOR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "XOR: " << i_text << std::endl;
                    }
                    break;
                }
                case SAL: {
                    size_t size = JIT::emmit_SHIFT_OP<PQP::SAL>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_SAL(++pqp_addr_counter[PC], PC, i.Rx, i.i5);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "SAL: " << i_text << std::endl;
                    }
                    break;
                }
                case SAR:{
                    size_t size = JIT::emmit_SHIFT_OP<PQP::SAR>(i, raw_code);
                    code_block.insert(code_block.end(), raw_code, raw_code + size);

                    if (mapped_instructions.count(PC)) {
                        mapped_instructions[PC].first += 1;
                    } else {
                        std::stringstream s;
                        s << PRINT_SAR(++pqp_addr_counter[PC], PC, i.Rx, i.i5);
                        mapped_instructions[PC] = {1, s.str()};
                        std::string i_text = JIT::to_string(raw_code, size);
                        std::cout << "SAR: " << i_text << std::endl;
                    }
                    break;
                }
            
            }

            PC += 4;

        }

        

        memcpy(&executable_memory[executable_memory_index], code_block.begin().base(), code_block.size());
        uint8_t* target_pc_address = &executable_memory[executable_memory_index];
        executable_memory_index += code_block.size();
        block_map[guest_PC] = target_pc_address;
        BlockMeta meta = {
            guest_PC,
            counter,
            (uint32_t*)(uintptr_t) counter_pointer
            
        };
        block_metadata_table.push_back(meta);
        //print_memory_dump(executable_memory, executable_memory_index);
        return target_pc_address;
    }

    void jit_handler (int sig, siginfo_t* info, void* ctx){

        mcontext_t* mcontext = &(((ucontext_t*) ctx)->uc_mcontext);
        //std::cout << "Valor no xmm2: " << (uint32_t) mcontext->fpregs->_xmm[2].element[0] << std::endl;

        uint32_t guestPC = mcontext->fpregs->_xmm[1].element[0];
        uint8_t* trap_address = ((uint8_t*) mcontext->gregs[REG_RIP] - 1);

        uint8_t* target_pc_address = NULL;

        

        if (block_map.find(guestPC) == block_map.end()){
            
            std::cout << "Novo bloco!" << std::endl;
            target_pc_address = interpreter(guestPC);
        }
        else{
            target_pc_address = block_map[guestPC];
        }

        

        int32_t offset = (int32_t) (target_pc_address - trap_address - 5);

        size_t s_block_channing = emmit_raw_blockchanning_jmp(raw_code, offset);
        memcpy(trap_address, raw_code, s_block_channing);

        mcontext->gregs[REG_RIP] = (greg_t) target_pc_address;

        print_memory_dump(executable_memory, executable_memory_index);



    }

    void jit_exit (int sig, siginfo_t* info, void* ctx){
        exit(0);
    }

    void set_initial_config (){

        alstack.ss_sp = malloc(SIGSTKSZ);
        alstack.ss_size = SIGSTKSZ;
        alstack.ss_flags = 0;
        sigaltstack(&alstack, NULL);

        actionINT.sa_handler = NULL;
        actionINT.sa_sigaction = jit_handler;
        sigemptyset(&actionINT.sa_mask);
        actionINT.sa_flags = SA_SIGINFO | SA_ONSTACK;

        sigaction(SIGTRAP, &actionINT, NULL);

        actionSEGFAUT.sa_handler = NULL;
        actionSEGFAUT.sa_sigaction = jit_exit; // Por enquanto segfault intencional
        sigemptyset(&actionSEGFAUT.sa_mask);
        actionSEGFAUT.sa_flags = SA_SIGINFO | SA_ONSTACK;

        sigaction(SIGSEGV, &actionSEGFAUT, NULL);

        executable_memory = (uint8_t*) mmap(
            (void*) 0x100000, 
            4*1024, 
            PROT_READ | PROT_WRITE | PROT_EXEC, 
            MAP_32BIT | MAP_PRIVATE | MAP_ANONYMOUS, 
            -1, 
        0);

        block_counters_arena = (uint32_t*) mmap(
            NULL, 4096, PROT_READ | PROT_WRITE, 
            MAP_32BIT | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
        );

        memset(block_counters_arena, 0, 4096);

        pc_constants_arena = (uint32_t*) mmap(
            NULL, 4096, PROT_READ | PROT_WRITE, 
            MAP_32BIT | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
        );
        
        for (uint32_t i = 0; i < 256; i++) {
            pc_constants_arena[i] = i; 
        }

        pc_constants_arena[1000] = 1;

    }

    void start_jitting (){

        JIT::set_initial_config();
        using JIT_FUNC = void (*) (void);
        JIT_FUNC f = (JIT_FUNC) executable_memory;
        uint8_t gen_code [16];
        size_t g1 = emmit_raw_xorps_xmm0_xmm0(gen_code);
        memcpy(executable_memory, gen_code, g1);
        size_t g2 = emmit_raw_xmm2_constant1(gen_code, (uint32_t) ((uint64_t) &pc_constants_arena[1000]));
        memcpy(&executable_memory[g1], gen_code, g2);
        size_t g3 = emmit_raw_xorps_xmm1_xmm1(gen_code);
        memcpy(&executable_memory[g1+g2], gen_code, g3);

        size_t start = g1 + g2 + g3;
        print_memory_dump(executable_memory, start);

        uint8_t zero_regs[] = {
            0x31, 0xC0,             // xor eax, eax
            0x31, 0xC9,             // xor ecx, ecx
            0x31, 0xD2,             // xor edx, edx
            0x31, 0xDB,             // xor ebx, ebx
            0x31, 0xE4,             // xor esp, esp
            0x31, 0xED,             // xor ebp, ebp
            0x31, 0xF6,             // xor esi, esi
            0x31, 0xFF,             // xor edi, edi
            0x45, 0x31, 0xC0,       // xor r8d, r8d
            0x45, 0x31, 0xC9,       // xor r9d, r9d
            0x45, 0x31, 0xD2,       // xor r10d, r10d
            0x45, 0x31, 0xDB,       // xor r11d, r11d
            0x45, 0x31, 0xE4,       // xor r12d, r12d
            0x45, 0x31, 0xED,       // xor r13d, r13d
            0x45, 0x31, 0xF6,       // xor r14d, r14d
            0x45, 0x31, 0xFF        // xor r15d, r15d
        };
        
        memcpy(&executable_memory[start], zero_regs, sizeof(zero_regs));
        start += sizeof(zero_regs);
        
        start += emmit_raw_jmp_placeholder(&executable_memory[start]);
        
        executable_memory_index = start;

        print_memory_dump(executable_memory, start);

        std::cout << "Começando JITTING..." << std::endl;
        f();


    }




}

std::ifstream input_file;
std::ofstream output_file;

int main (int argc, char* argv[]){
    
    memset(PQP::pqp_regs, 0, 16 * sizeof(int));

    if (argc > 1) input_file = std::ifstream (argv[1]);
    if (argc > 2) output_file = std::ofstream (argv[2]);

    PQP::load_memory(input_file);
    JIT::start_jitting();

    //JIT::interpreter();

    for (auto& [addr, counter] : PQP::mapped_instructions){
        if (addr < 256) output_file << LINE_HEADER(counter.first, addr) << counter.second << std::endl;
        else{
            output_file << "       -:" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)addr << ":exit" << std::endl;
        }
        
    }

    output_file << '\n';

    for (int j = 0; j < 15; j++){
           output_file << PRINT_REG(j) << PQP::pqp_regs[j] << ',';
       }

       output_file << PRINT_REG(15) << PQP::pqp_regs[15] << std::endl;

    return 0;
}