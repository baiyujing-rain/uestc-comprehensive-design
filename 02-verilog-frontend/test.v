module test(
    input s, a, b,
    output y
);

wire mux_out;
wire expr_out;
wire rewrite_out;

assign mux_out = s ? a : a;
assign expr_out = (b & 1'b1) | 1'b0;
assign rewrite_out = a | (a & b);
assign y = mux_out ^ expr_out ^ rewrite_out;

endmodule
