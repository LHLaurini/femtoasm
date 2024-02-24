"use strict";

import rr from './railroad-diagrams/railroad.js';

var Choice = rr.Choice;
var Comment = rr.Comment;
var Diagram = rr.Diagram;
var End = rr.End;
var Group = rr.Group;
var HorizontalChoice = rr.HorizontalChoice;
var NonTerminal = rr.NonTerminal;
var OneOrMore = rr.OneOrMore;
var Optional = rr.Optional;
var Sequence = rr.Sequence;
var Skip = rr.Skip;
var Start = rr.Start;
var ZeroOrMore = rr.ZeroOrMore;

function reference(name) {
	return () => NonTerminal(name, { href: `#${name}` });
}

const nonSeparator = reference("non-separator");
const hexadecimal = reference("hexadecimal");
const separator = reference("separator");
const simpleStart = label => Start({ label: label });
const complexStart = label => Start({ type: "complex", label: label });
const complexEnd = () => End({ type: "complex" });

const diagrams = [
	Diagram(
		complexStart("non-separator"),
		NonTerminal("any character greater than ' '"),
		complexEnd()
	),
	Diagram(
		complexStart("separator"),
		NonTerminal("any character less than or equal to ' '"),
		complexEnd()
	),
	Diagram(
		simpleStart("hexadecimal"),
		HorizontalChoice(
			Choice(1,
				"0",
				"1",
				"2",
				"3"
			),
			Choice(1,
				"4",
				"5",
				"6",
				"7"
			),
			Choice(1,
				"8",
				"9",
				"A",
				"B"
			),
			Choice(1,
				"C",
				"D",
				"E",
				"F"
			)
		)
	),
	Diagram(
		complexStart("file"),
		ZeroOrMore(
			Choice(0,
				Sequence(
					"=",
					Group(
						ZeroOrMore(nonSeparator()),
						"identifier"
					),
					separator(),
					Group(
						OneOrMore(
							hexadecimal(),
							Comment("up to 8 digits")
						),
						"value"
					)
				),
				Sequence(
					"$",
					Group(
						ZeroOrMore(nonSeparator()),
						"identifier"
					),
					separator(),
				),
				Sequence(
					"%",
					Group(
						OneOrMore(
							hexadecimal(),
							Comment("up to 16 digits")
						),
						"value"
					),
					separator()
				),
				Sequence(
					NonTerminal("other"),
					Comment("ignored")
				)
			)
		),
		complexEnd()
	),
];

window.addEventListener('load', () => {
	var diagramsElement = document.getElementById("diagrams");

	if (diagramsElement === null) {
		console.error("No 'diagrams' element.");
		return;
	}

	for (var diagram of diagrams) {
		let anchor = document.createElement("a");
		let label = diagram.items[0].label;
		anchor.setAttribute("name", label);
		diagramsElement.appendChild(anchor);

		let div = document.createElement("div");
		div.classList.add("diagram_container");
		if (label == "file") {
			div.classList.add("main");
		}
		diagram.addTo(diagramsElement.appendChild(div));
	}
});
