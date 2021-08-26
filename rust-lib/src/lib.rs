use libc::{c_char, size_t};
use std::ffi::CStr;

pub fn get_byte_hash(bytes: &[u8]) -> [u8;64] {
    openssl::sha::sha512( bytes )
}

#[no_mangle]
pub extern "C" fn get_hash(data: *const c_char, hash_buffer: *mut u8) -> size_t {   /*returns length of final hash, expecting enough size of hash_buffer, ie. 64 bytes, also note hash_buffer is a byte array*/
    if data.is_null() { return 0; }

    let c_str: &CStr = unsafe { // Before any action on raw *char, convert to CStr first
        CStr::from_ptr(data)
    };

    let c_str_bytes = c_str.to_bytes();
    let hashed_result = get_byte_hash(c_str_bytes); 

    unsafe {
        std::ptr::copy(hashed_result.as_ptr(), hash_buffer, hashed_result.len());   // 64 bytes
    };

    hashed_result.len()
}

#[no_mangle]
#[deprecated]
pub extern "C" fn sign_str(_data: *const c_char, _signed_msg_buffer: *mut c_char, _public_key_buffer: *mut c_char) -> size_t { // Returns msg_buffer size, PREVIOUSLY: (Signed_Data, Signer_public_key)
    
    unimplemented!();

}

