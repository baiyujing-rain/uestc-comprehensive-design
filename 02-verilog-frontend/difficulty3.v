module difficulty3(
    input s, t, a, b, c, d, e,
    output y
);

wire nested_mux;
wire bool_mux;
wire expr_a;
wire rewrite_a;
wire rewrite_b;
wire shared1;
wire shared2;

assign nested_mux = s ? (s ? a : b) : (s ? c : d);
assign bool_mux = t ? 1'b1 : 1'b0;
assign expr_a = ((b & 1'b1) | 1'b0) ^ (c ^ c);
assign rewrite_a = ~(a & e) | (d & 1'b0);
assign rewrite_b = (a & b) | (a & d);
assign shared1 = (a & b) ^ (c | d);
assign shared2 = (a & b) ^ (c | d);
assign y = nested_mux ^ bool_mux ^ expr_a ^ rewrite_a ^ rewrite_b ^ shared1 ^ shared2;

endmodule
