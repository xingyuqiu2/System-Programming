/**
 * vector
 * CS 241 - Spring 2021
 */
#include "sstring.h"
#include <stdio.h>

void p_elements(vector* v) {
    printf("[");
    size_t i = 0;
    if (i < vector_size(v)) printf("%s", vector_get(v, i));
    for (i = 1; i < vector_size(v); i++) {
        printf(",%s", vector_get(v, i));
    }
    printf("]\n");
}

int main(int argc, char *argv[]) {
    // TODO create some tests

    //test sstring_append
    sstring *first_ss = cstr_to_sstring("abc");
    sstring *second_ss = cstr_to_sstring("def");
    int len = sstring_append(first_ss, second_ss); // len == 6
    printf("%d\n", len);
    char* newstr = sstring_to_cstr(first_ss);
    printf("%s\n", newstr); // == "abcdef"
    free(newstr);
    sstring_destroy(first_ss);
    sstring_destroy(second_ss);

    //test sstring_split
    sstring* ss1 = cstr_to_sstring("eabecdeeghe"); //['', 'ab', 'cd', '', 'gh', '']
    char* str1 = sstring_to_cstr(ss1);
    printf("%s\n", str1);
    free(str1);
    vector* v1 = sstring_split(ss1, 'e');
    p_elements(v1);
    vector_destroy(v1);
    sstring_destroy(ss1);

    sstring* ss2 = cstr_to_sstring("This is a sentence.");
    char* str2 = sstring_to_cstr(ss2);
    printf("%s\n", str2);
    free(str2);
    vector* v2 = sstring_split(ss2, ' ');
    p_elements(v2);
    vector_destroy(v2);
    sstring_destroy(ss2);

    //test sstring_substitute with target less than the substitution
    sstring *replace_me = cstr_to_sstring("This is a {} day, {}");
    sstring_substitute(replace_me, 18, "{}", "friend!");
    char* str3 = sstring_to_cstr(replace_me);
    printf("%s/\n", str3);
    free(str3);
    sstring_substitute(replace_me, 0, "{}", "good");
    str3 = sstring_to_cstr(replace_me);
    printf("%s/\n", str3);
    free(str3);
    sstring_substitute(replace_me, 0, "{}", "oh");
    str3 = sstring_to_cstr(replace_me);
    printf("%s/\n", str3);
    free(str3);
    sstring_destroy(replace_me);

    //test sstring_substitute with target longer than the substitution
    sstring *replace_me2 = cstr_to_sstring("When is a {...} day, {...}!");
    sstring_substitute(replace_me2, 16, "{...}", "yeah");
    char* str4 = sstring_to_cstr(replace_me2);
    printf("%s/\n", str4);
    free(str4);
    sstring_substitute(replace_me2, 0, "{...}", "good");
    str4 = sstring_to_cstr(replace_me2);
    printf("%s/\n", str4);
    free(str4);
    sstring_substitute(replace_me2, 0, "When", "Today");
    str4 = sstring_to_cstr(replace_me2);
    printf("%s/\n", str4);
    free(str4);
    sstring_destroy(replace_me2);

    //test sstring_slice
    sstring *slice_me = cstr_to_sstring("1234567890");
    char* str_sli = sstring_slice(slice_me, 2, 5);
    printf("%s\n", str_sli); // == "345"
    free(str_sli);
    sstring_destroy(slice_me);
    return 0;
}
