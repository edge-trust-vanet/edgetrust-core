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
        fontSize=18,
        leading=22,
        textColor=colors.black,
        spaceAfter=2
    )

    team_style = ParagraphStyle(
        'TeamTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=colors.black,
        spaceAfter=4
    )

    h2_style = ParagraphStyle(
        'H2',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=13,
        leading=17,
        textColor=colors.black,
        spaceBefore=16,
        spaceAfter=8,
        keepWithNext=True
    )

    bullet_style = ParagraphStyle(
        'Bullet',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=10,
        leading=15,
        textColor=colors.black,
        leftIndent=15,
        firstLineIndent=-10,
        spaceAfter=8.5
    )

    story = []

    # Title & Team
    story.append(Paragraph("Current Progress and Next Steps", title_style))
    story.append(Paragraph("Team 43", team_style))
    story.append(HRFlowable(width="100%", thickness=1, color=colors.black, spaceBefore=4, spaceAfter=10))

    # Completed Work
    story.append(Paragraph("Completed Work", h2_style))

    completed_points = [
        "Integrated SUMO, OMNeT++ and Veins.",
        "Implemented V2V/V2I communication and OBU-based heuristic filtering.",
        "Integrated RSU-based ML detection and trust management.",
        "Embedded trained ML models (Random Forest and Logistic Regression) as native C++ predictors in the RSU for real-time edge inference.",
        "Implemented approximately 15 attack scenarios aligned with VeReMi categories.",
        "Completed 14-feature extraction and CSV logging.",
        "Added trust-score updates and detection visualization.",
        "Implemented simulation and trust-state checkpointing.",
        "Set up a two-RSU simulation scenario.",
        "Implemented inter-RSU (I2I) communication backbone for periodic trust synchronization and urgent threat alerts.",
        "Implemented regional Fog layer between RSUs with multi-RSU trust aggregation.",
        "Handled duplicate, stale and conflicting trust updates with Bayesian consensus and safety override.",
        "Built an interactive web dashboard for real-time model evaluation and data inspection."
    ]

    for pt in completed_points:
        story.append(Paragraph(f"• &nbsp; {pt}", bullet_style))

    story.append(Spacer(1, 8))

    # Remaining Work
    story.append(Paragraph("Remaining Work", h2_style))

    remaining_points = [
        "Integrate SDN-based adaptive forwarding (By 16th October: SDN based adaptive forwarding for multiple RSUs).",
        "Test scalability with more RSUs and larger network scenarios (By 18th October)."
    ]

    for pt in remaining_points:
        story.append(Paragraph(f"• &nbsp; {pt}", bullet_style))

    doc.build(story, canvasmaker=SimpleNumberedCanvas)
    print("Done")

if __name__ == '__main__':
    build_pdf('/tmp/bw_report.pdf')
