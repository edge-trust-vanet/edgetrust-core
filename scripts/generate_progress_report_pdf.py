import os
import sys
from reportlab.lib.pagesizes import letter
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, HRFlowable
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.pdfgen import canvas

class SimpleNumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.pages = []

    def showPage(self):
        self.pages.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self.pages)
        for page in self.pages:
            self.__dict__.update(page)
            self.draw_footer(num_pages)
            super().showPage()
        super().save()

    def draw_footer(self, total_pages):
        self.saveState()
        self.setFont("Helvetica", 9)
        self.setFillColor(colors.black)
        self.drawString(54, 34, "EdgeTrust-VANET Project — Team 43")
        if total_pages > 1:
            self.drawRightString(letter[0] - 54, 34, f"Page {self._pageNumber} of {total_pages}")
        self.setStrokeColor(colors.black)
        self.setLineWidth(0.5)
        self.line(54, 46, letter[0] - 54, 46)
        self.restoreState()

def build_pdf(filename):
    doc = SimpleDocTemplate(
        filename,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=48,
        bottomMargin=54
    )

    styles = getSampleStyleSheet()

    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=15,
        leading=19,
        textColor=colors.black,
        spaceAfter=2
    )

    team_style = ParagraphStyle(
        'TeamTitle',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=10,
        leading=14,
        textColor=colors.black,
        spaceAfter=3
    )

    h2_style = ParagraphStyle(
        'H2',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=11,
        leading=15,
        textColor=colors.black,
        spaceBefore=10,
        spaceAfter=5,
        keepWithNext=True
    )

    bullet_style = ParagraphStyle(
        'Bullet',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=9,
        leading=13,
        textColor=colors.black,
        leftIndent=12,
        firstLineIndent=-8,
        spaceAfter=3.5
    )

    story = []

    # Title & Team
    story.append(Paragraph("Current Progress and Next Steps", title_style))
    story.append(Paragraph("Team 43", team_style))
    story.append(HRFlowable(width="100%", thickness=0.75, color=colors.black, spaceBefore=3, spaceAfter=8))

    # Completed Work
    story.append(Paragraph("Completed Work", h2_style))

    completed_points = [
        "Integrated SUMO, OMNeT++ and Veins co-simulation environment.",
        "Implemented V2V/V2I communication and OBU-based heuristic filtering.",
        "Integrated RSU-based ML detection and trust management.",
        "Embedded trained ML models (Random Forest and Logistic Regression) as native C++ predictors in the RSU for real-time edge inference.",
        "Implemented approximately 15 attack scenarios aligned with VeReMi categories.",
        "Completed 14-feature extraction and CSV logging pipeline.",
        "Added trust-score updates and live detection visualization.",
        "Implemented simulation and trust-state checkpointing.",
        "Expanded to a multi-grid 4-RSU scenario (rsu[0..3]) across two interconnected intersections.",
        "Implemented inter-RSU (I2I) communication backbone for periodic trust synchronization and urgent threat alerts.",
        "Implemented multi-tier Fog layer (fog[0..1]) with regional ledger aggregation, conflict resolution (Bayesian consensus and safety-critical override), and inter-Fog synchronization.",
        "Integrated SDN-based adaptive forwarding with Dijkstra trust-cost routing and 3-class priority QoS scheduling.",
        "Built an interactive web dashboard for real-time model evaluation and data inspection."
    ]

    for pt in completed_points:
        story.append(Paragraph(f"• &nbsp; {pt}", bullet_style))

    story.append(Spacer(1, 4))

    # Remaining Work
    story.append(Paragraph("Remaining Work", h2_style))

    remaining_points = [
        "Test scalability with more RSUs and larger network scenarios (Target: 18th October).",
        "Evaluate accuracy, precision, recall, F1-score, false-positive rate, latency, and packet delivery ratio across diverse traffic densities."
    ]

    for pt in remaining_points:
        story.append(Paragraph(f"• &nbsp; {pt}", bullet_style))

    doc.build(story, canvasmaker=SimpleNumberedCanvas)
    print("Done")

if __name__ == '__main__':
    out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bw_report.pdf'
    build_pdf(out_path)
