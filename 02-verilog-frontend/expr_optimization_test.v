module expr_optimization_test(
    input a, b, c,
    output y
);

wire w_keep_a;
wire w_keep_c;
wire w_invert_a;
wire w_final;

assign w_keep_a = (a & 1'b1) | 1'b0;
assign w_keep_c = (b ^ b) | (c & c);
assign w_invert_a = a ^ 1'b1;
assign w_final = (w_keep_a & w_keep_c) | w_invert_a;
assign y = w_final;

endmodule
