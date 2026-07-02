#!/usr/bin/env python3
"""Parse a PDF file and output content stream tokens to stdout.

Usage:
    python pdf_tokens.py <input.pdf> [page_numbers...]

Output format (one token per line):
    operand1 operand2 ... operandN operator
    operator                        (when no operands)

With no page numbers specified, all pages are processed.
"""

import sys
from io import BytesIO
from pdfminer.pdfparser import PDFParser
from pdfminer.pdfdocument import PDFDocument
from pdfminer.pdfpage import PDFPage
from pdfminer.pdfinterp import PDFContentParser
from pdfminer.psparser import PSKeyword, PSLiteral, keyword_name, literal_name
from pdfminer.pdftypes import PDFStream
from pdfminer.psexceptions import PSEOF


def format_operand(obj):
    """Format a single operand to its PDF content stream text representation."""
    if isinstance(obj, (int, float)):
        # Numbers: output as-is. PDF uses decimal numbers.
        return str(obj)
    elif isinstance(obj, PSLiteral):
        # Name object (e.g., /F1, /DeviceRGB)
        name = literal_name(obj)
        return "/" + name
    elif isinstance(obj, PSKeyword):
        # This shouldn't normally appear as an operand, but handle it
        name = keyword_name(obj)
        return name
    elif isinstance(obj, bool):
        return "true" if obj else "false"
    elif obj is None:
        return "null"
    elif isinstance(obj, bytes):
        # Literal string — PDF strings can contain arbitrary bytes
        return "(" + obj.decode("latin-1", errors="replace") + ")"
    elif isinstance(obj, list):
        # Array (e.g., dash pattern [2 3])
        return "[" + " ".join(format_operand(x) for x in obj) + "]"
    elif isinstance(obj, dict):
        # Dictionary (<< ... >>) — rare in content streams but possible
        parts = []
        for k, v in obj.items():
            parts.append(format_operand(k) + " " + format_operand(v))
        return "<<" + " ".join(parts) + ">>"
    elif isinstance(obj, PDFStream):
        # Inline image — output a placeholder
        return "BI…EI"
    else:
        return str(obj)


def process_page(page, out):
    """Parse and output all tokens from a single PDF page."""
    parser = PDFContentParser(list(page.contents))
    operands = []

    while True:
        try:
            (_, obj) = parser.nextobject()
        except PSEOF:
            break

        if isinstance(obj, PSKeyword):
            # This is an operator — flush accumulated operands
            name = keyword_name(obj)
            if operands:
                out.write(" ".join(format_operand(op) for op in operands))
                out.write(" " + name + "\n")
            else:
                out.write(name + "\n")
            operands.clear()
        else:
            # This is an operand
            operands.append(obj)

    # In case of malformed stream with trailing operands (shouldn't happen)
    if operands:
        out.write(" ".join(format_operand(op) for op in operands) + "\n")


def main():
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        sys.exit(1)

    pdf_path = sys.argv[1]
    page_numbers = None
    if len(sys.argv) > 2:
        page_numbers = set()
        for arg in sys.argv[2:]:
            try:
                page_numbers.add(int(arg))
            except ValueError:
                print(f"Invalid page number: {arg}", file=sys.stderr)
                sys.exit(1)

    with open(pdf_path, "rb") as f:
        parser = PDFParser(f)
        doc = PDFDocument(parser)

        for page_idx, page in enumerate(PDFPage.create_pages(doc), start=1):
            if page_numbers is not None and page_idx not in page_numbers:
                continue
            process_page(page, sys.stdout)


if __name__ == "__main__":
    main()
