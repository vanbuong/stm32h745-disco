function(target_append_coverage tgt)
    target_compile_options(${tgt} PRIVATE --coverage -O0 -g)
    target_link_options(${tgt} PRIVATE --coverage)
endfunction()
