#pragma once

#include "ScriptNum.hpp"
#include "OpEnum.hpp"
#include "utils.hpp"

#include <iostream>
#include <vector>
#include <stack>
#include <iomanip>
#include <unordered_map>

static const int32_t  MAX_SCRIPT_SIZE          = 10000;
static const int32_t  MAX_OP_PER_SCRIPT        = 201;
static const int32_t  MAX_STACK_SIZE           = 1000;
static const uint32_t MAX_STACK_ELEMENT_SIZE   = 520;
static const int32_t  MAX_PUBKEYS_PER_MULTISIG = 20;
static const uint32_t LOCKTIME_THRESHOLD       = 500000000;
static const uint32_t LOCKTIME_MAX             = 0xFFFFFFFFU;

class Script;
using OperationPointer = void (Script::*)(void);
using OperationPair    = std::pair<std::string, OperationPointer>;
using OperationHashMap = std::unordered_map<OpEnum, OperationPair>;

class Script {
public:
    Script() {}
    explicit Script(const std::string& hex): exec(hex2bytes(hex)) {}
    explicit Script(const std::vector<uint8_t>& bytes): exec(bytes) {}
    
    // Script Execution
    std::vector<uint8_t> Run();

    // Script Insertion / Serialization
    Script& operator<<(const OpEnum& op);
    Script& operator<<(const int32_t& num);
    Script& operator<<(const std::vector<uint8_t>& data);

    std::vector<uint8_t> GetBytes() const { return exec; };

    friend auto& operator<<(std::ostream& out, const Script& script) {
        for (auto& b: script.exec)
            out << std::hex << std::setfill('0') << std::setw(2) << +b << "";
        return out;
    }

private:
    // The stack and alt-stack used during the script execution.
    std::vector<std::vector<uint8_t>> stack;
    std::vector<std::vector<uint8_t>> alt_stack;
    // The script serialized bytes.
    std::vector<uint8_t> exec;
    // Stack of boolean used for nested conditional blocks.
    std::stack<bool> run_this; 
    // Hashmap of OpEnum -> pair(str(OP), function_ptr(OP));
    static OperationHashMap ScriptInterface;

    void Interpreter(const std::vector<uint8_t>& exec);
    bool CastAsBool(std::vector<uint8_t> bytes)      const;
    void CheckStack(size_t n, const char* _func_)    const;
    void CheckAltStack(size_t n, const char* _func_) const;
    void DisabledOperation(const char* _func_)       const;
    void InvalidOperation(const char* _func_)        const;

    ////////////////// SCRIPT OPERATIONS //////////////////

    // Constants

    void op_0();
    void op_1();
    void op_2();
    void op_3();
    void op_4();
    void op_5();
    void op_6();
    void op_7();
    void op_8();
    void op_9();
    void op_10();
    void op_11();
    void op_12();
    void op_13();
    void op_14();
    void op_15();
    void op_16();

    void op_false();
    void op_true();

    void op_pushadata1();
    void op_pushadata2();
    void op_pushadata4();
    void op_1negate();

    // Flow Control

    void op_nop();
    void op_if();
    void op_notif();
    void op_else();
    void op_endif();
    void op_verify();
    void op_return(); //

    // Stack
    
    void op_toaltstack();
    void op_fromaltstack();
    void op_ifdup();
    void op_depth();
    void op_drop();
    void op_dup();
    void op_nip();
    void op_over();
    void op_pick();
    void op_roll();
    void op_rot();
    void op_swap();
    void op_tuck();
    void op_2drop();
    void op_2dup();
    void op_3dup();
    void op_2over();
    void op_2rot();
    void op_2swap();

    // Splice

    void op_cat();
    void op_substr();
    void op_left();
    void op_right();
    void op_size();

    // Bitwise Logic
    
    void op_invert();
    void op_and();
    void op_or();
    void op_xor();
    void op_equal();
    void op_equalverify();

    // Arithmetic

    void op_1add();
    void op_1sub();
    void op_2mul();
    void op_2div();
    void op_negate();
    void op_abs();
    void op_not();
    void op_0notequal();
    void op_add();
    void op_sub();
    void op_mul();
    void op_div();
    void op_mod();
    void op_lshift();
    void op_rshift();
    void op_booland();
    void op_boolor();
    void op_numequal();
    void op_numequalverify();
    void op_numnotequal();
    void op_lessthan();
    void op_greaterthan();
    void op_lessthanorequal();
    void op_greaterthanorequal();
    void op_min();
    void op_max();
    void op_within();

    // Crypto

    void op_ripemd160();
    void op_sha1();
    void op_sha256();
    void op_hash160();
    void op_hash256();
    void op_codeseparator(); //
    void op_checksig(); //
    void op_checksigverify(); //
    void op_checkmultisigverify(); //

    // Locktime
    
    void op_checklocktimeverify(); //
    void op_checksequenceverify(); //
    void op_nop2();
    void op_nop3();

    // Pseudo-words

    void op_pubkeyhash();
    void op_pubkey();
    void op_invalidopcode();

    // Reserverd words

    void op_reserved();
    void op_ver();
    void op_verif();
    void op_vernotif();
    void op_reserved1();
    void op_reserved2();
    void op_nop1();
    void op_nop4();
    void op_nop5();
    void op_nop6();
    void op_nop7();
    void op_nop8();
    void op_nop9();
    void op_nop10();

    /////////////////////////////////////////////////////////
};
