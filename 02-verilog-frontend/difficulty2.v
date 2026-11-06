module difficulty2(
    input s, a, b, c, d,
    output y
);

wire mux_keep;
wire expr_keep;
wire rewrite_keep;
wire factor_keep;

assign mux_keep = s ? (s ? a : b) : c;
assign expr_keep = ((b ^ b) | c) & 1'b1;
assign rewrite_keep = a | (a & d);
assign factor_keep = (a & b) | (a & c);
assign y = mux_keep ^ expr_keep ^ rewrite_keep ^ factor_keep;

endmodule
