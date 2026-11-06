module mux_optimization_test(
    input s, t, a, b, c, d,
    output y
);

wire same_mux;
wire const_mux;
wire bool_mux;
wire nested_mux;

assign same_mux = s ? a : a;
assign const_mux = 1'b0 ? b : c;
assign bool_mux = t ? 1'b1 : 1'b0;
assign nested_mux = s ? (s ? a : b) : (s ? c : d);
assign y = nested_mux ^ same_mux ^ const_mux ^ bool_mux;

endmodule
