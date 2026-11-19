export var zfoo_value: i32 = 5;
export var zfoo_ptr: *i32 = &zfoo_value;
export fn zfoo_add(a: i32) i32 {
    return a + zfoo_value;
}
