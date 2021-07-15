#include "Script.hpp"
#include "hashes.hpp"

////////////////////////////// SCRIPT EXECUTION /////////////////////////////

// Run the script and return last element of stack.
std::vector<uint8_t> Script::Run() {
    if (exec.empty())
        throw std::runtime_error("Run: cannot run empty script");
    if (exec.size() > MAX_SCRIPT_SIZE)
        throw std::runtime_error("Run: script size > 10000 bytes");
    
    // All OPs should be executed by default.
    run_this.push(true);
    
    // Do the thing
    Interpreter(exec);
    
    // Check and clean up.    
    if(!stack.empty()) {
        auto result = stack.back();
        stack.clear();
        alt_stack.clear();
        run_this = std::stack<bool>(); // might not be necessary
        return result;
    } else throw std::runtime_error("Run: script resulted in empty stack");
}

void Script::Interpreter(const std::vector<uint8_t>& script) {
    
    using Iterator = std::vector<uint8_t>::const_iterator;

    auto num_op = 0;

    // Fetch data and increment script iterator.
    auto fetch_data = [](const size_t& data_size, Iterator& script_it) {
        std::vector<uint8_t> data;
        data.reserve(data_size);
        for (size_t i = 0; i < data_size; i++)
            data.push_back(*(++script_it));
        return data;
    };

    // Skip conditional block and increment script iterator.
    auto skip_block = [&script, fetch_data](Iterator& script_it) {
        auto nested_if = 0;
        for (;script_it != script.end(); script_it++) {
            if (*script_it >= 0x01 && *script_it <= 0x4b) // skip data
                fetch_data(*script_it, script_it);
            else if (*script_it == OP_PUSHDATA1) // skip data
                fetch_data(*script_it, ++script_it);
            else if (*script_it == OP_PUSHDATA2) { // skip data
                std::vector<uint8_t> size_bytes{*(++script_it), *(++script_it)};
                ScriptNum size(size_bytes);
                if (size > MAX_STACK_ELEMENT_SIZE)
                    throw std::runtime_error("Interpreter: data size > 520 bytes");
                fetch_data(ScriptNum(size).GetInt(), script_it);
            }
            if (*script_it == OP_IF || *script_it == OP_NOTIF) nested_if++;
            if ((*script_it == OP_ELSE || *script_it == OP_ENDIF) && !nested_if)
                return;
            if (*script_it == OP_ENDIF && nested_if) nested_if--;
        } throw std::runtime_error("Interpreter: missing op_endif");
    };

    // Interpreter main loop.
    for (auto script_it = script.begin(); script_it != script.end(); script_it++) {
        auto op_name = ScriptInterface[(OpEnum)*script_it].first;
        auto op_ptr  = ScriptInterface[(OpEnum)*script_it].second;
        if (op_ptr != nullptr && ++num_op > MAX_OP_PER_SCRIPT)
            throw std::runtime_error("Interpreter: reached OPs limit (201)");
        if (stack.size() > MAX_STACK_SIZE)
            throw std::runtime_error("Interpreter: reached max stack size (1000)");
        if (!op_name.empty())
            std::cout << op_name << std::endl;

        // For conditional blocks that should be evaluated.
        if ((*script_it == OP_IF || *script_it == OP_NOTIF) && run_this.top()) {
            CheckStack(1, "conditional_block:");
            if (CastAsBool(stack.back())) // Evaluate condition.
                 run_this.push(*script_it == OP_IF    ? true : false);
            else run_this.push(*script_it == OP_NOTIF ? true : false);
            stack.pop_back(); // Pop condition.
            if (!run_this.top()) skip_block(++script_it); // Skip block when cond == false.
        }

        // These OPs shouldn't appear if we're not in a conditional block.
        if (*script_it == OP_ELSE || *script_it == OP_ENDIF) {
            if (run_this.size() != 1) {
                if (*script_it == OP_ELSE)
                    run_this.top() = !run_this.top();
                else if (*script_it == OP_ENDIF)
                    run_this.pop();
            } else throw std::runtime_error("Interpreter: missing op_if");
        }

        // Run all other OPs.
        if (run_this.top()) {
            if (*script_it >= 0x01 && *script_it <= 0x4b) // data bytelength
                stack.push_back(fetch_data(*script_it, script_it));
            else if (*script_it == OP_PUSHDATA1) // next byte = data bytelength
                stack.push_back(fetch_data(*script_it, ++script_it));
            else if (*script_it == OP_PUSHDATA2) { // next two bytes = data bytelength
                std::vector<uint8_t> size_bytes{*(++script_it), *(++script_it)};
                ScriptNum size(size_bytes);
                if (size > MAX_STACK_ELEMENT_SIZE)
                    throw std::runtime_error("Interpreter: data size > 520 bytes");
                stack.push_back(fetch_data(ScriptNum(size).GetInt(), script_it));
            } else if (op_ptr != nullptr) std::invoke(op_ptr, this); //(this->*op_ptr)(); // run normal operations
            else InvalidOperation(__func__);
        }
    }
    
    if (run_this.size() > 1)
        throw std::runtime_error("Interpreter: missing op_endif");

    std::cout << "\nSTACK\n";
    for (auto& s: stack)
        std::cout << " - [" << toHex(s) << "] " << s.size() << std::endl;
    std::cout << "END_STACK\n";
}

//////////////////////////////////////////////////////////////////////////////

////////////////////////////// SCRIPT INSERTION //////////////////////////////

Script& Script::operator<<(const int32_t& n) {
    ScriptNum num(n);
    exec.push_back((uint8_t)num.GetBytes().size());
    for (auto& b: num.GetBytes())
        exec.push_back(b);
    return *this;
}

Script& Script::operator<<(const OpEnum& op) {
    exec.push_back((uint8_t)op);
    return *this;
}

Script& Script::operator<<(const std::vector<uint8_t>& data) {
    size_t size = data.size();
    
    if (size <= 75)
        exec.push_back((uint8_t)size);
    else if (size >= 76 && size <= 255) {
        exec.push_back(OpEnum::OP_PUSHDATA1);
        exec.push_back((uint8_t)size);
    } else if (size >= 256 && size <= MAX_STACK_ELEMENT_SIZE) {
        exec.push_back(OpEnum::OP_PUSHDATA2);
        for (auto& byteLE: ScriptNum(size).GetBytes())
            exec.push_back(byteLE);
    } else throw std::runtime_error("data size > 520 bytes");
    
    for (auto& byte: data)
        exec.push_back(byte);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////

////////////////////////////// HELPER FUNCTIONS //////////////////////////////

bool Script::CastAsBool(std::vector<uint8_t> bytes) const {
    return std::find_if(bytes.begin(), bytes.end(), 
        [](uint8_t x) { return x != 0x00; }) 
        != bytes.end();
}

void Script::CheckStack(size_t n, const char* _func_) const{
    size_t size = stack.size();
    if (size < n) {
        std::stringstream error;
        error << _func_ << ": ";
        if (size == 0) error << "stack empty";
        else error << "stack size < " << n;
        throw std::runtime_error(error.str());
    }
}

void Script::CheckAltStack(size_t n, const char* _func_) const {
    size_t size = alt_stack.size();
    if (size < n) {
        std::stringstream error;
        error << _func_ << ": ";
        if (size == 0) error << "alt-stack empty";
        else error << "alt-stack size < " << n;
        throw std::runtime_error(error.str());
    }
}

void Script::DisabledOperation(const char* _func_) const {
    std::stringstream error;
    error << _func_ << ": operation disabled";
    throw std::runtime_error(error.str());
}

void Script::InvalidOperation(const char* _func_) const {
    std::stringstream error;
    error << _func_ << ": invalid operation";
    throw std::runtime_error(error.str());
}

////////////////////////////// SCRIPT OPERATIONS /////////////////////////////

///////////////////////////////// CONSTANTS //////////////////////////////////

void Script::op_false() { op_0(); }
void Script::op_true()  { op_1(); }

void Script::op_0()  { stack.push_back(ScriptNum(0).GetBytes());  } 
void Script::op_1()  { stack.push_back(ScriptNum(1).GetBytes());  }
void Script::op_2()  { stack.push_back(ScriptNum(2).GetBytes());  }
void Script::op_3()  { stack.push_back(ScriptNum(3).GetBytes());  }
void Script::op_4()  { stack.push_back(ScriptNum(4).GetBytes());  }
void Script::op_5()  { stack.push_back(ScriptNum(5).GetBytes());  }
void Script::op_6()  { stack.push_back(ScriptNum(6).GetBytes());  }
void Script::op_7()  { stack.push_back(ScriptNum(7).GetBytes());  }
void Script::op_8()  { stack.push_back(ScriptNum(8).GetBytes());  }
void Script::op_9()  { stack.push_back(ScriptNum(9).GetBytes());  }
void Script::op_10() { stack.push_back(ScriptNum(10).GetBytes()); }
void Script::op_11() { stack.push_back(ScriptNum(11).GetBytes()); }
void Script::op_12() { stack.push_back(ScriptNum(12).GetBytes()); }
void Script::op_13() { stack.push_back(ScriptNum(13).GetBytes()); }
void Script::op_14() { stack.push_back(ScriptNum(14).GetBytes()); }
void Script::op_15() { stack.push_back(ScriptNum(15).GetBytes()); }
void Script::op_16() { stack.push_back(ScriptNum(16).GetBytes()); }

void Script::op_pushadata1() { return; } // dealt with in Interpreter
void Script::op_pushadata2() { return; } // dealt with in Interpreter
void Script::op_pushadata4() { return; } // dealt with in Interpreter

void Script::op_1negate() { stack.push_back(ScriptNum(-1).GetBytes()); }

//////////////////////////////////////////////////////////////////////////////


/////////////////////////////// FLOW CONTROL /////////////////////////////////

void Script::op_nop()    { return; } // does nothing
void Script::op_if()     { return; } // dealt with in Interpreter
void Script::op_notif()  { return; } // dealt with in Interpreter
void Script::op_else()   { return; } // dealt with in Interpreter
void Script::op_endif()  { return; } // dealt with in Interpreter

// Marks transaction as invalid if top stack value is not true (pop stack).
void Script::op_verify() {
    CheckStack(1, __func__);
    if (!CastAsBool(stack.back())) 
        throw std::runtime_error("op_verify: invalid");
    stack.pop_back();
}

// i don't fucking know
void Script::op_return() {}

///////////////////////////////////////////////////////////////////////////////


/////////////////////////////////// STACK ////////////////////////////////////

// Puts the input onto the top of the alt stack. Removes it from the main stack. 
void Script::op_toaltstack() { 
    CheckStack(1, __func__);
    alt_stack.push_back(stack.back()); 
    stack.pop_back();
}

// Puts the input onto the top of the main stack. Removes it from the alt stack. 
void Script::op_fromaltstack() { 
    CheckAltStack(1, __func__);
    stack.push_back(alt_stack.back());
    alt_stack.pop_back();
}

// If the top stack value is not 0, duplicate it. 
void Script::op_ifdup() {
    CheckStack(1, __func__);
    if (CastAsBool(stack.back()))
        op_dup();
}

// Puts the number of stack items onto the stack. 
void Script::op_depth() {
    CheckStack(1, __func__);
    stack.push_back(ScriptNum(stack.size()).GetBytes());
}

// Removes the top stack item. 
void Script::op_drop() {
    CheckStack(1, __func__);
    stack.pop_back();
}

// Duplicates the top stack item. 
void Script::op_dup() {
    CheckStack(1, __func__);
    stack.push_back(stack.back());
}

// Removes the second-to-top stack item. 
void Script::op_nip() {
    CheckStack(2, __func__);
    stack.erase(stack.end()-2);
}

// Copies the second-to-top stack item to the top. 
void Script::op_over() {
    CheckStack(2, __func__);
    stack.push_back(stack.end()[-2]);
}

// The item n back in the stack is copied to the top.
void Script::op_pick() {
    CheckStack(1, __func__);
    auto num = ScriptNum(stack.back()).GetInt();
    stack.pop_back();
    CheckStack(num, __func__);
    stack.push_back(stack.end()[-num]);
}

// The item n back in the stack is moved to the top.
void Script::op_roll() {
    CheckStack(1, __func__);
    auto num = ScriptNum(stack.back()).GetInt();
    stack.pop_back();
    CheckStack(num, __func__);
    auto val = std::move(stack.end()[-num]);
    stack.erase(stack.end()-num);
    stack.push_back(val);
}

// The 3rd item down the stack is moved to the top. 
void Script::op_rot() {
    CheckStack(3, __func__);
    auto val = std::move(stack.end()[-3]);
    stack.erase(stack.end()-3);
    stack.push_back(val);
}

// The top two items on the stack are swapped. 
void Script::op_swap() {
    CheckStack(2, __func__);
    std::swap(stack.end()[-1], stack.end()[-2]);
}

// The item at the top of the stack is copied and inserted before the second-to-top item. 
void Script::op_tuck() {
    CheckStack(2, __func__);
    stack.insert(stack.end()-2, stack.end()[-1]);
}

// Removes the top two stack items. 
void Script::op_2drop() {
    CheckStack(2, __func__);
    stack.pop_back();
    stack.pop_back();
}

// Duplicates the top two stack items.
void Script::op_2dup() {
    CheckStack(2, __func__);
    stack.push_back(stack.end()[-2]);
    stack.push_back(stack.end()[-2]);
}

// Duplicates the top three stack items.
void Script::op_3dup() {
    CheckStack(3, __func__);
    stack.push_back(stack.end()[-3]);
    stack.push_back(stack.end()[-3]);
    stack.push_back(stack.end()[-3]);
}

// Copies the pair of items two spaces back in the stack to the front. 
void Script::op_2over() {
    CheckStack(4, __func__);
    stack.push_back(stack.end()[-4]);
    stack.push_back(stack.end()[-4]);
}

// The fifth and sixth items back are moved to the top of the stack. 
void Script::op_2rot() {
    CheckStack(6, __func__);
    auto val1 = std::move(stack.end()[-6]);
    auto val2 = std::move(stack.end()[-5]);
    stack.erase(stack.end()-6, stack.end()-4);
    stack.push_back(val1);
    stack.push_back(val2);
}

// Swaps the top two pairs of items. 
void Script::op_2swap() {
    CheckStack(4, __func__);
    std::swap(stack.end()[-4], stack.end()[-2]);
    std::swap(stack.end()[-3], stack.end()[-1]);
}

//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////// SPLICE ///////////////////////////////////

void Script::op_cat()    { DisabledOperation(__func__); }
void Script::op_substr() { DisabledOperation(__func__); }
void Script::op_left()   { DisabledOperation(__func__); } 
void Script::op_right()  { DisabledOperation(__func__); } 

// Pushes the string length of the top element of the stack.
void Script::op_size() {
    CheckStack(1, __func__);
    stack.push_back(ScriptNum(stack.back().size()).GetBytes());
}

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////// BITWISE LOGIC ///////////////////////////////

void Script::op_invert() { DisabledOperation(__func__); }
void Script::op_and()    { DisabledOperation(__func__); }
void Script::op_or()     { DisabledOperation(__func__); }
void Script::op_xor()    { DisabledOperation(__func__); }

// Returns 1 if the inputs are exactly equal, 0 otherwise. 
void Script::op_equal() {
    CheckStack(2, __func__);
    auto rhs = stack.back(); stack.pop_back();
    auto lhs = stack.back(); stack.pop_back();
    if (lhs == rhs)
         stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Same as OP_EQUAL, but runs OP_VERIFY afterward.
void Script::op_equalverify() {
    op_equal();
    op_verify();
}

//////////////////////////////////////////////////////////////////////////////


////////////////////////////////// ARITHMETIC ////////////////////////////////

// 1 is added to the input.
void Script::op_1add() {
    CheckStack(1, __func__);
    ScriptNum num(stack.back()); stack.pop_back();
    stack.push_back((num+1).GetBytes());
}

// 1 is subtracted from the input. 
void Script::op_1sub() {
    CheckStack(1, __func__);
    ScriptNum num(stack.back()); stack.pop_back();
    stack.push_back((num-1).GetBytes());
}

void Script::op_2mul() { DisabledOperation(__func__); }
void Script::op_2div() { DisabledOperation(__func__); }

// The sign of the input is flipped. 
void Script::op_negate() {
    CheckStack(1, __func__);
    ScriptNum num(stack.back()); stack.pop_back();
    stack.push_back((-num).GetBytes());
}

// The input is made positive. 
void Script::op_abs() {
    CheckStack(1, __func__);
    ScriptNum num(stack.back()); stack.pop_back();
    stack.push_back((num > 0 ? num : -num).GetBytes());
}

// If the input is 0 or 1, it is flipped. Otherwise the output will be 0.
void Script::op_not() {
    CheckStack(1, __func__);
    ScriptNum num(stack.back()); stack.pop_back();
    if (num == 0) stack.push_back(ScriptNum(1).GetBytes());
    else if (num == 1) stack.push_back(ScriptNum(0).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Returns 0 if the input is 0. 1 otherwise. 
void Script::op_0notequal() {
    CheckStack(1, __func__);
    ScriptNum num(stack.back());
    if (num != 0) { 
        stack.pop_back();
        stack.push_back(ScriptNum(1).GetBytes());
    }
}

// lhs is added to rhs.
void Script::op_add() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    stack.push_back((lhs+rhs).GetBytes());
}

// rhs is subtracted from lhs. 
void Script::op_sub() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    stack.push_back((lhs-rhs).GetBytes());
}

void Script::op_mul()    { DisabledOperation(__func__); }
void Script::op_div()    { DisabledOperation(__func__); }
void Script::op_mod()    { DisabledOperation(__func__); }
void Script::op_lshift() { DisabledOperation(__func__); } 
void Script::op_rshift() { DisabledOperation(__func__); } 

// If both lhs and rhs are not 0, the output is 1. Otherwise 0.
void Script::op_booland() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs != 0 && rhs != 0) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// If lhs or rhs is not 0, the output is 1. Otherwise 0. 
void Script::op_boolor() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs != 0 || rhs != 0) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Returns 1 if the numbers are equal, 0 otherwise.
void Script::op_numequal() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs == rhs) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Same as OP_NUMEQUAL, but runs OP_VERIFY afterward.
void Script::op_numequalverify() {
    op_numequal();
    op_verify();
}

// Returns 1 if the numbers are not equal, 0 otherwise.
void Script::op_numnotequal() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs != rhs) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Returns 1 if lhs is less than rhs, 0 otherwise. 
void Script::op_lessthan() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs < rhs) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Returns 1 if lhs is greater than rhs, 0 otherwise.
void Script::op_greaterthan() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs > rhs) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Returns 1 if lhs is less than or equal to rhs, 0 otherwise.
void Script::op_lessthanorequal() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs <= rhs) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Returns 1 if lhs is greater than or equal to rhs, 0 otherwise. 
void Script::op_greaterthanorequal() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    if (lhs >= rhs) stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

// Returns the smaller of lhs and rhs. 
void Script::op_min() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    stack.push_back(lhs < rhs ? lhs.GetBytes() : rhs.GetBytes());
}

// Returns the larger of lhs and rhs.
void Script::op_max() {
    CheckStack(2, __func__);
    ScriptNum rhs(stack.back()); stack.pop_back();
    ScriptNum lhs(stack.back()); stack.pop_back();
    stack.push_back(lhs > rhs ? lhs.GetBytes() : rhs.GetBytes());
}

// Returns 1 if x is within the specified range (left-inclusive), 0 otherwise. 
void Script::op_within() {
    CheckStack(3, __func__);
    ScriptNum max(stack.back()); stack.pop_back();
    ScriptNum min(stack.back()); stack.pop_back();
    ScriptNum val(stack.back()); stack.pop_back();
    if (val >= min && val < max) 
        stack.push_back(ScriptNum(1).GetBytes());
    else stack.push_back(ScriptNum(0).GetBytes());
}

//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////// CRYPTO ///////////////////////////////////

// The input is hashed using RIPEMD-160. 
void Script::op_ripemd160() {
    CheckStack(1, __func__);
    auto hash = ripemd160(stack.back());
    stack.pop_back();
    stack.push_back(hash);
}

// The input is hashed using SHA-1. 
void Script::op_sha1() {
    CheckStack(1, __func__);
    auto hash = sha1(stack.back());
    stack.pop_back();
    stack.push_back(hash);
}

// The input is hashed using SHA-256. 
void Script::op_sha256() {
    CheckStack(1, __func__);
    auto hash = sha256(stack.back());
    stack.pop_back();
    stack.push_back(hash);
}

// The input is hashed twice: first with SHA-256 and then with RIPEMD-160. 
void Script::op_hash160() {
    CheckStack(1, __func__);
    auto hash = hash160(stack.back());
    stack.pop_back();
    stack.push_back(hash);
}

// The input is hashed two times with SHA-256. 
void Script::op_hash256() {
    CheckStack(1, __func__);
    auto hash = hash256(stack.back());
    stack.pop_back();
    stack.push_back(hash);
}

/*  All of the signature checking words will only match signatures to 
    the data after the most recently-executed OP_CODESEPARATOR. */
void Script::op_codeseparator() {

}

/* The entire transaction's outputs, inputs, and script 
   (from the most recently-executed OP_CODESEPARATOR to the end) are hashed. 
   The signature used by OP_CHECKSIG must be a valid signature for this hash
   and public key. 
   If it is, 1 is returned, 0 otherwise. */
void Script::op_checksig() {

}

// Same as OP_CHECKSIG, but OP_VERIFY is executed afterward. 
void Script::op_checksigverify() {

}

/* Compares the first signature against each public key until it finds 
   an ECDSA match.
   Starting with the subsequent public key, it compares the second 
   signature against each remaining public key until it finds an ECDSA match. 
   The process is repeated until all signatures have been checked or 
   not enough public keys remain to produce a successful result. 
   All signatures need to match a public key. 
   Because public keys are not checked again if they fail any signature 
   comparison, signatures must be placed in the scriptSig using the same 
   order as their corresponding public keys were placed in the scriptPubKey 
   or redeemScript. 
   If all signatures are valid, 1 is returned, 0 otherwise. 
   Due to a bug, one extra unused value is removed from the stack. */
void Script::op_checkmultisigverify() {

}

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////// LOCKTIME ////////////////////////////////

/* Marks transaction as invalid if the top stack item is greater than the
   transaction's nLockTime field, otherwise script evaluation continues as 
   though an OP_NOP was executed. 
   Transaction is also invalid if:
   1. the stack is empty;
   2. the top stack item is negative; 
   3. the top stack item is greater than or equal to 500000000 while the 
   transaction's nLockTime field is less than 500000000, or vice versa;
   4. the input's nSequence field is equal to 0xffffffff.
   The precise semantics are described in BIP-0065:
   https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki. */
void Script::op_checklocktimeverify() {

}

/* Marks transaction as invalid if the relative lock time of the input 
   (enforced by BIP 0068 with nSequence) is not equal to or longer than
   the value of the top stack item. 
   The precise semantics are described in BIP-0112:
   https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki */
void Script::op_checksequenceverify() {

}

void Script::op_nop2() { op_checklocktimeverify(); }
void Script::op_nop3() { op_checksequenceverify(); }

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////// PSEUDO-WORDS ////////////////////////////////

void Script::op_pubkey()        { InvalidOperation(__func__); }
void Script::op_pubkeyhash()    { InvalidOperation(__func__); }
void Script::op_invalidopcode() { InvalidOperation(__func__); }

//////////////////////////////////////////////////////////////////////////////


/////////////////////////////// RESERVED WORDS ///////////////////////////////

// Transaction is invalid unless occuring in an unexecuted OP_IF branch 
void Script::op_reserved() {
    if (!(run_this.size() > 1 && !run_this.top()))
        InvalidOperation(__func__);
}
    
// Transaction is invalid unless occuring in an unexecuted OP_IF branch 
void Script::op_ver() {
    if (!(run_this.size() > 1 && !run_this.top()))
        InvalidOperation(__func__);
} 

// Transaction is invalid even when occuring in an unexecuted OP_IF branch
void Script::op_verif() {
    InvalidOperation(__func__);
} 

// Transaction is invalid even when occuring in an unexecuted OP_IF branch 
void Script::op_vernotif() {
    InvalidOperation(__func__);
}

// Transaction is invalid unless occuring in an unexecuted OP_IF branch
void Script::op_reserved1() {
    if (!(run_this.size() > 1 && !run_this.top()))
        InvalidOperation(__func__);
}

// Transaction is invalid unless occuring in an unexecuted OP_IF branch 
void Script::op_reserved2() {
    if (!(run_this.size() > 1 && !run_this.top()))
        InvalidOperation(__func__);
}

void Script::op_nop1()  { return; } // does nothing 
void Script::op_nop4()  { return; } // does nothing
void Script::op_nop5()  { return; } // does nothing
void Script::op_nop6()  { return; } // does nothing
void Script::op_nop7()  { return; } // does nothing
void Script::op_nop8()  { return; } // does nothing
void Script::op_nop9()  { return; } // does nothing
void Script::op_nop10() { return; } // does nothing

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


/////////////////////////// OPERATION HASHMAP ///////////////////////////

OperationHashMap Script::ScriptInterface = 

OperationHashMap {
    
    // Constants
    {OP_0,                  {"OP_0",                    &Script::op_0}},
    {OP_1,                  {"OP_1",                    &Script::op_1}},
    {OP_2,                  {"OP_2",                    &Script::op_2}},
    {OP_3,                  {"OP_3",                    &Script::op_3}},
    {OP_4,                  {"OP_4",                    &Script::op_4}},
    {OP_5,                  {"OP_5",                    &Script::op_5}},
    {OP_6,                  {"OP_6",                    &Script::op_6}},
    {OP_7,                  {"OP_7",                    &Script::op_7}},
    {OP_8,                  {"OP_8",                    &Script::op_8}},
    {OP_9,                  {"OP_9",                    &Script::op_9}},
    {OP_10,                 {"OP_10",                   &Script::op_10}},
    {OP_11,                 {"OP_11",                   &Script::op_11}},
    {OP_12,                 {"OP_12",                   &Script::op_12}},
    {OP_13,                 {"OP_13",                   &Script::op_13}},
    {OP_14,                 {"OP_14",                   &Script::op_14}},
    {OP_15,                 {"OP_15",                   &Script::op_15}},
    {OP_16,                 {"OP_16",                   &Script::op_16}},    
    {OP_FALSE,              {"OP_FALSE",                &Script::op_false}},
    {OP_TRUE,               {"OP_TRUE",                 &Script::op_true}},
    {OP_PUSHDATA1,          {"OP_PUSHDATA1",            &Script::op_pushadata1}},
    {OP_PUSHDATA2,          {"OP_PUSHDATA2",            &Script::op_pushadata2}},
    {OP_PUSHDATA4,          {"OP_PUSHDATA4",            &Script::op_pushadata4}},
    {OP_1NEGATE,            {"OP_1NEGATE",              &Script::op_1negate}},

    // Flow Control
    {OP_NOP,                {"OP_NOP",                  &Script::op_nop}},
    {OP_IF,                 {"OP_IF",                   &Script::op_if}},
    {OP_NOTIF,              {"OP_NOTIF",                &Script::op_notif}},
    {OP_ELSE,               {"OP_ELSE",                 &Script::op_else}},
    {OP_ENDIF,              {"OP_ENDIF",                &Script::op_endif}},
    {OP_VERIFY,             {"OP_VERIFY",               &Script::op_verify}},
    {OP_RETURN,             {"OP_RETURN",               &Script::op_return}},

    // Stack
    {OP_TOALTSTACK,          {"OP_TOALTSTACK",          &Script::op_toaltstack}},
    {OP_FROMALTSTACK,        {"OP_FROMALTSTACK",        &Script::op_fromaltstack}},
    {OP_IFDUP,               {"OP_IFDUP",               &Script::op_ifdup}},
    {OP_DEPTH,               {"OP_DEPTH",               &Script::op_depth}},
    {OP_DROP,                {"OP_DROP",                &Script::op_drop}},
    {OP_DUP,                 {"OP_DUP",                 &Script::op_dup}},
    {OP_NIP,                 {"OP_NIP",                 &Script::op_nip}},
    {OP_OVER,                {"OP_OVER",                &Script::op_over}},
    {OP_PICK,                {"OP_PICK",                &Script::op_pick}},
    {OP_ROLL,                {"OP_ROLL",                &Script::op_roll}},
    {OP_ROT,                 {"OP_ROT",                 &Script::op_rot}},
    {OP_SWAP,                {"OP_SWAP",                &Script::op_swap}},
    {OP_TUCK,                {"OP_TUCK",                &Script::op_tuck}},
    {OP_2DROP,               {"OP_2DROP",               &Script::op_2drop}},
    {OP_2DUP,                {"OP_2DUP",                &Script::op_2dup}},
    {OP_3DUP,                {"OP_3DUP",                &Script::op_3dup}},
    {OP_2OVER,               {"OP_2OVER",               &Script::op_2over}},
    {OP_2ROT,                {"OP_2ROT",                &Script::op_2rot}},
    {OP_2SWAP,               {"OP_2SWAP",               &Script::op_2swap}},

    // Splice
    {OP_CAT,                 {"OP_CAT",                 &Script::op_cat}},
    {OP_SUBSTR,              {"OP_SUBSTR",              &Script::op_substr}},
    {OP_LEFT,                {"OP_LEFT",                &Script::op_left}},
    {OP_RIGHT,               {"OP_RIGHT",               &Script::op_right}},
    {OP_SIZE,                {"OP_SIZE",                &Script::op_size}},

    // Bitwise Logic
    {OP_INVERT,              {"OP_INVERT",              &Script::op_invert}},
    {OP_AND,                 {"OP_AND",                 &Script::op_and}},
    {OP_OR,                  {"OP_OR",                  &Script::op_or}},
    {OP_XOR,                 {"OP_XOR",                 &Script::op_xor}},
    {OP_EQUAL,               {"OP_EQUAL",               &Script::op_equal}},
    {OP_EQUALVERIFY,         {"OP_EQUALVERIFY",         &Script::op_equalverify}},

    // Arithmetic
    {OP_1ADD,                {"OP_1ADD",                &Script::op_1add}},
    {OP_1SUB,                {"OP_1SUB",                &Script::op_1sub}},
    {OP_2MUL,                {"OP_2MUL",                &Script::op_2mul}},
    {OP_2DIV,                {"OP_2DIV",                &Script::op_2div}},
    {OP_NEGATE,              {"OP_NEGATE",              &Script::op_negate}},
    {OP_ABS,                 {"OP_ABS",                 &Script::op_abs}},
    {OP_NOT,                 {"OP_NOT",                 &Script::op_not}},
    {OP_0NOTEQUAL,           {"OP_0NOTEQUAL",           &Script::op_0notequal}},
    {OP_ADD,                 {"OP_ADD",                 &Script::op_add}},
    {OP_SUB,                 {"OP_SUB",                 &Script::op_sub}},
    {OP_MUL,                 {"OP_MUL",                 &Script::op_mul}},
    {OP_DIV,                 {"OP_DIV",                 &Script::op_div}},
    {OP_MOD,                 {"OP_MOD",                 &Script::op_mod}},
    {OP_LSHIFT,              {"OP_LSHIFT",              &Script::op_lshift}},
    {OP_RSHIFT,              {"OP_RSHIFT",              &Script::op_rshift}},
    {OP_BOOLAND,             {"OP_BOOLAND",             &Script::op_booland}},
    {OP_BOOLOR,              {"OP_BOOLOR",              &Script::op_boolor}},
    {OP_NUMEQUAL,            {"OP_NUMEQUAL",            &Script::op_numequal}},
    {OP_NUMEQUALVERIFY,      {"OP_NUMEQUALVERIFY",      &Script::op_numequalverify}},
    {OP_NUMNOTEQUAL,         {"OP_NUMNOTEQUAL",         &Script::op_numnotequal}},
    {OP_LESSTHAN,            {"OP_LESSTHAN",            &Script::op_lessthan}},
    {OP_GREATERTHAN,         {"OP_GREATHERTHAN",        &Script::op_greaterthan}},
    {OP_LESSTHANOREQUAL,     {"OP_LESSTHANOREQUAL",     &Script::op_lessthanorequal}},
    {OP_GREATERTHANOREQUAL,  {"OP_GREATHERTHANOREQUAL", &Script::op_greaterthanorequal}},
    {OP_MIN,                 {"OP_MIN",                 &Script::op_min}},
    {OP_MAX,                 {"OP_MAX",                 &Script::op_max}},
    {OP_WITHIN,              {"OP_WITHIN",              &Script::op_within}},


    // Crypto
    {OP_RIPEMD160,           {"OP_RIPEMD160",           &Script::op_ripemd160}},
    {OP_SHA1,                {"OP_SHA1",                &Script::op_sha1}},
    {OP_SHA256,              {"OP_SHA256",              &Script::op_sha256}},
    {OP_HASH160,             {"OP_HASH160",             &Script::op_hash160}},
    {OP_HASH256,             {"OP_HASH256",             &Script::op_hash256}},
    {OP_CODESEPARATOR,       {"OP_CODESEPARATOR",       &Script::op_codeseparator}},
    {OP_CHECKSIG,            {"OP_CHECKSIG",            &Script::op_checksig}},
    {OP_CHECKSIGVERIFY,      {"OP_CHECKSIGVERIFY",      &Script::op_checksigverify}},
    {OP_CHECKMULTISIGVERIFY, {"OP_CHECKMULTISIGVERIFY", &Script::op_checkmultisigverify}},

    // Locktime
    {OP_CHECKLOCKTIMEVERIFY, {"OP_CHECKLOCKTIMEVERIFY", &Script::op_checklocktimeverify}},
    {OP_CHECKSEQUENCEVERIFY, {"OP_CHECKSEQUENCEVERIFY", &Script::op_checksequenceverify}},
    {OP_NOP2,                {"OP_NOP2",                &Script::op_nop2}},
    {OP_NOP3,                {"OP_NOP3",                &Script::op_nop3}},

    // Pseudo-words
    {OP_PUBKEYHASH,          {"OP_PUBKEYHASH",          &Script::op_pubkeyhash}},
    {OP_PUBKEY,              {"OP_PUBKEY",              &Script::op_pubkey}},
    {OP_INVALIDOPCODE,       {"OP_INVALIDOPCODE",       &Script::op_invalidopcode}},

    // Reserved words
    {OP_RESERVED,            {"OP_RESERVED",            &Script::op_reserved}},
    {OP_VER,                 {"OP_VER",                 &Script::op_ver}},
    {OP_VERIF,               {"OP_VERIF",               &Script::op_verif}},
    {OP_VERNOTIF,            {"OP_VERNOTIF",            &Script::op_vernotif}},
    {OP_RESERVED1,           {"OP_RESERVED1",           &Script::op_reserved1}},
    {OP_RESERVED2,           {"OP_RESERVED2",           &Script::op_reserved2}},
    {OP_NOP1,                {"OP_NOP1",                &Script::op_nop1}},
    {OP_NOP4,                {"OP_NOP4",                &Script::op_nop4}},
    {OP_NOP5,                {"OP_NOP5",                &Script::op_nop5}},
    {OP_NOP6,                {"OP_NOP6",                &Script::op_nop6}},
    {OP_NOP7,                {"OP_NOP7",                &Script::op_nop7}},
    {OP_NOP8,                {"OP_NOP8",                &Script::op_nop8}},
    {OP_NOP9,                {"OP_NOP9",                &Script::op_nop9}},
    {OP_NOP10,               {"OP_NOP10",               &Script::op_nop10}}
};
