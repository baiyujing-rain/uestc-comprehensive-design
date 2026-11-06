module difficulty1(
    input a, b,
    output y
);

wire t;

assign t = a & 1'b1;
assign y = t | b;

endmodule
