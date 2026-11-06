module rewrite_optimization_test(
    input a, b, c, d,
    output y
);

wire demorgan_and;
wire demorgan_or;
wire absorb_or;
wire absorb_and;
wire factor_out;
wire double_not;

assign demorgan_and = ~(a & b);
assign demorgan_or = ~(c | d);
assign absorb_or = a | (a & c);
assign absorb_and = b & (b | d);
assign factor_out = (a & c) | (a & d);
assign double_not = ~~c;
assign y = demorgan_and ^ demorgan_or ^ absorb_or ^ absorb_and ^ factor_out ^ double_not;

endmodule
