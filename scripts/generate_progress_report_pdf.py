import os
import sys
from reportlab.lib.pagesizes import letter
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    """
    Two-pass canvas to dynamically compute and display total page count,
    running headers, and professional running footers.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_page_decorations(self, page_count):
        self.saveState()
        self.setFont("Helvetica-Bold", 8)
        self.setFillColor(colors.HexColor("#718096"))
        
        # Running Header on pages > 1
        if self._pageNumber > 1:
            self.drawString(54, 752, "EdgeTrust-VANET: Hierarchical Trust-Adaptive Secure Communication")
            self.setFont("Helvetica", 8)
            self.drawRightString(letter[0] - 54, 752, "Project Status & Technical Report")
            self.setStrokeColor(colors.HexColor("#CBD5E0"))
            self.setLineWidth(0.75)
            self.line(54, 744, letter[0] - 54, 744)
            
        # Running Footer on all pages
        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#718096"))
        page_text = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(letter[0] - 54, 32, page_text)
        self.drawString(54, 32, "EdgeTrust Research & Engineering Report | Department of Computer Science & Engineering")
        self.setStrokeColor(colors.HexColor("#CBD5E0"))
        self.setLineWidth(0.75)
        self.line(54, 44, letter[0] - 54, 44)
        
        self.restoreState()

def build_pdf(filename):
    doc = SimpleDocTemplate(
        filename,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )
    
    # ── Professional Color Palette ─────────────────────────────────────────────
    c_primary = colors.HexColor("#1A365D")      # Deep Navy
    c_secondary = colors.HexColor("#2B6CB0")    # Slate Blue
    c_accent_green = colors.HexColor("#22543D") # Forest Green
    c_accent_amber = colors.HexColor("#744210") # Amber / Orange
    c_text_dark = colors.HexColor("#2D3748")    # Charcoal Dark
    c_text_muted = colors.HexColor("#4A5568")   # Neutral Grey
    c_bg_light = colors.HexColor("#F7FAFC")     # Soft Off-White
    c_bg_subtle = colors.HexColor("#EDF2F7")    # Table Header Grey
    c_bg_alt = colors.HexColor("#FAFCFE")       # Alternating Row Tint
    c_border = colors.HexColor("#E2E8F0")       # Light Border
    c_card_green = colors.HexColor("#F0FFF4")   # Light Green Card
    c_card_amber = colors.HexColor("#FFFAF0")   # Light Amber Card
    c_card_blue = colors.HexColor("#EBF8FF")    # Light Blue Card

    # ── Typography & Styles ───────────────────────────────────────────────────
    styles = getSampleStyleSheet()
    
    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=18,
        leading=22,
        textColor=c_primary,
        spaceAfter=3
    )
    
    subtitle_style = ParagraphStyle(
        'DocSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=10.5,
        leading=14,
        textColor=c_secondary,
        spaceAfter=10
    )
    
    meta_style = ParagraphStyle(
        'DocMeta',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11,
        textColor=c_text_muted
    )
    
    h1_style = ParagraphStyle(
        'H1',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=c_primary,
        spaceBefore=12,
        spaceAfter=4,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'H2',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=9.8,
        leading=13,
        textColor=c_secondary,
        spaceBefore=8,
        spaceAfter=3,
        keepWithNext=True
    )
    
    body_style = ParagraphStyle(
        'Body',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11.2,
        textColor=c_text_dark,
        spaceAfter=4
    )

    bullet_style = ParagraphStyle(
        'BulletText',
        parent=body_style,
        leftIndent=10,
        firstLineIndent=-7,
        spaceAfter=2.5
    )

    callout_text = ParagraphStyle(
        'CalloutText',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.8,
        leading=11,
        textColor=c_text_dark
    )
    
    th_style = ParagraphStyle(
        'TH',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=7.2,
        leading=9,
        textColor=c_primary,
        alignment=0
    )

    td_style = ParagraphStyle(
        'TD',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7,
        leading=8.8,
        textColor=c_text_dark
    )

    td_bold = ParagraphStyle(
        'TDBold',
        parent=td_style,
        fontName='Helvetica-Bold'
    )

    status_done = ParagraphStyle(
        'StatusDone',
        parent=td_style,
        fontName='Helvetica-Bold',
        textColor=c_accent_green
    )

    status_prog = ParagraphStyle(
        'StatusProg',
        parent=td_style,
        fontName='Helvetica-Bold',
        textColor=c_accent_amber
    )

    story = []
    
    # ── Header Banner ──────────────────────────────────────────────────────────
    story.append(Paragraph("EdgeTrust-VANET: Hierarchical Trust-Adaptive Secure Communication Framework", title_style))
    story.append(Paragraph("Final Year Project (Capstone) — Comprehensive Progress & Engineering Roadmap", subtitle_style))
    
    meta_table_data = [
        [
            Paragraph("<b>Project Domain:</b> Connected Autonomous Vehicles (V2X) & Edge Security", meta_style),
            Paragraph("<b>Repository:</b> <code>edgetrust-core</code> & <code>edgetrust-ml</code>", meta_style),
            Paragraph("<b>Date:</b> October 2026", meta_style)
        ],
        [
            Paragraph("<b>Simulation Stack:</b> SUMO 1.22.0 + OMNeT++ 6.4.0 + Veins 5.2", meta_style),
            Paragraph("<b>Edge AI Engine:</b> Random Forest & Logistic Regression (C++ Native)", meta_style),
            Paragraph("<b>Status:</b> Phases 1 & 2 Complete (Phase 3 Active)", meta_style)
        ]
    ]
    meta_table = Table(meta_table_data, colWidths=[180, 204, 120])
    meta_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), c_bg_light),
        ('BOX', (0,0), (-1,-1), 0.75, c_border),
        ('INNERGRID', (0,0), (-1,-1), 0.5, c_border),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('LEFTPADDING', (0,0), (-1,-1), 6),
        ('RIGHTPADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(meta_table)
    story.append(Spacer(1, 8))

    # ── Executive Overview Card ────────────────────────────────────────────────
    exec_summary_text = (
        "<b>Executive Summary:</b> EdgeTrust-VANET delivers a multi-tiered, defense-in-depth cybersecurity "
        "and dynamic trust management framework for Connected Vehicles. The architecture seamlessly integrates "
        "On-Board Unit (OBU) heuristic pre-filtering, Roadside Unit (RSU) native Edge AI classification, dynamic Bayesian-EMA "
        "trust scoring, and Inter-RSU (I2I) cooperative threat quarantine over an edge backbone. This document details all "
        "completed engineering milestones (including extensive ML pipeline benchmarks and C++ native deployments) and establishes "
        "a rigorous roadmap for remaining Fog-level aggregation, SDN adaptive forwarding, and multi-density stress testing."
    )
    exec_table = Table([[Paragraph(exec_summary_text, callout_text)]], colWidths=[504])
    exec_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), c_card_blue),
        ('BOX', (0,0), (-1,-1), 1, c_secondary),
        ('TOPPADDING', (0,0), (-1,-1), 5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 5),
        ('LEFTPADDING', (0,0), (-1,-1), 8),
        ('RIGHTPADDING', (0,0), (-1,-1), 8),
    ]))
    story.append(exec_table)
    story.append(Spacer(1, 8))

    # ──────────────────────────────────────────────────────────────────────────
    # SECTION 1: COMPLETED WORK
    # ──────────────────────────────────────────────────────────────────────────
    story.append(Paragraph("1. Completed Work & Current Technical Achievements", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=c_primary, spaceBefore=1, spaceAfter=6))
    
    # Subsection 1.1: Core Co-Simulation Stack
    story.append(Paragraph("1.1 Integrated Simulation Stack & Network Architecture (SUMO + OMNeT++ + Veins)", h2_style))
    story.append(Paragraph(
        "• <b>Full Bi-Directional Co-Simulation:</b> Successfully coupled Eclipse SUMO 1.22.0 (microscopic multi-lane traffic) "
        "with OMNeT++ 6.4.0 (discrete event network simulation) through Veins 5.2 TraCIScenarioManager over a robust TCP socket interface (Port 9999).",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Physical & MAC Layer Conformance:</b> Deployed IEEE 802.11p / DSRC Wave wireless communication stack (10 MHz channel at 5.9 GHz, "
        "transmission power 20 mW, bitrate 6 Mbps, obstacle shadow fading, and receiver sensitivity -98 dBm).",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Two-Grid Road Network Topology:</b> Engineered an interconnected dual-intersection road network featuring Grid 1 (Junction J13, SUMO [-18.0, 8.0]) "
        "and Grid 2 (Junction J20, SUMO [102.0, 8.0]) bridged by a 120.31 m bidirectional arterial corridor (E10/-E10) with realistic cross-grid and turning flows.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Strict 85 m DSRC Radio Range Boundary & Sector Assignment:</b> Enforced realistic 85 m communication limits at physical decider and "
        "application levels. Implemented proximity-based serving assignment: nodes closer to RSU 0 are designated [PRIMARY SERVING] by RSU 0 (Grid 1 Sector), "
        "while RSU 1 tracks them as [HANDOVER OBSERVER] inside the overlapping transition corridor (93 m - 143 m).",
        bullet_style
    ))
    story.append(Spacer(1, 4))

    # Subsection 1.2: OBU Heuristics
    story.append(Paragraph("1.2 OBU-Based Local Heuristic Filtering (Vehicle-Tier First Line of Defense)", h2_style))
    story.append(Paragraph(
        "A 4-tier distributed heuristic filtering engine was implemented directly on vehicle On-Board Units (OBUs) to evaluate outgoing and "
        "incoming Basic Safety Messages (BSMs) in real time before forwarding:",
        body_style
    ))
    
    obu_table_data = [
        [Paragraph("Heuristic Tier", th_style), Paragraph("Evaluation Mechanism", th_style), Paragraph("Thresholds & Physical Boundaries", th_style), Paragraph("Attack Mitigated", th_style)],
        [
            Paragraph("Tier 1: Kinematic Plausibility", td_bold),
            Paragraph("Checks physical limits of urban vehicular motion.", td_style),
            Paragraph("Speed: 0 to 45 m/s (162 km/h)<br/>Accel: -8.5 to +6.0 m/s²<br/>Max implied velocity: 55 m/s", td_style),
            Paragraph("Constant/Random Position Offsets, Position Mirroring, Out-of-bounds speed.", td_style)
        ],
        [
            Paragraph("Tier 2: CAN Bus vs. BSM Cross-Check", td_bold),
            Paragraph("Compares internal vehicle odometry against reported broadcast data.", td_style),
            Paragraph("Discrepancy tolerance: |V_can - V_bsm| ≤ 2.5 m/s<br/>Static moving check: v_can > 1.5 m/s & v_bsm == 0", td_style),
            Paragraph("Zero Speed Report, Feigned Braking, Spoofed Sensor Telemetry.", td_style)
        ],
        [
            Paragraph("Tier 3: Directional & Heading Consistency", td_bold),
            Paragraph("Calculates spatial displacement vector and compares against reported compass heading.", td_style),
            Paragraph("Heading deviation: |Heading - MotionAngle| ≤ 110°<br/>Active only when vehicle speed > 1.5 m/s", td_style),
            Paragraph("Reversed Heading, Erratic FDI maneuvers, Ghost lane jumps.", td_style)
        ],
        [
            Paragraph("Tier 4: Rate & Comm Anomaly Filter", td_bold),
            Paragraph("Sliding 1.0s window monitoring transmission frequency and message delays.", td_style),
            Paragraph("Max burst rate: ≤ 12 BSM/sec<br/>Latency ceiling: ≤ 500 ms", td_style),
            Paragraph("DoS Broadcast Flooding (16 Hz), Stale Data Replay, Time Delay attacks.", td_style)
        ]
    ]
    obu_table = Table(obu_table_data, colWidths=[105, 135, 140, 124], repeatRows=1)
    obu_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), c_bg_subtle),
        ('GRID', (0,0), (-1,-1), 0.5, c_border),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, c_bg_alt]),
        ('TOPPADDING', (0,0), (-1,-1), 2.5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2.5),
        ('LEFTPADDING', (0,0), (-1,-1), 4),
        ('RIGHTPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(obu_table)
    story.append(Spacer(1, 3))
    story.append(Paragraph("<i>Impact: Obvious rogue messages are dropped locally on the OBU (0% transmitted to RSU), reducing edge network overhead by up to 68%.</i>", meta_style))
    story.append(Spacer(1, 5))

    # Subsection 1.3: edgetrust-ml Pipeline & Edge AI
    story.append(PageBreak())
    story.append(Paragraph("1.3 Machine Learning Pipeline & Native C++ Embedded Predictors (edgetrust-ml)", h2_style))
    story.append(Paragraph(
        "• <b>Dataset Unification (23,970 Records):</b> Merged 9 official VeReMi simulation archives (17,470 records, 72.88%), "
        "EdgeTrust-VANET live extractions (5,000 records, 20.86%), and V-RADD routing attacks (1,500 records, 6.26%) into a standardized dataset.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Zero-Leakage Architectural Audit:</b> Conducted Pearson correlation, Spearman rank, and Mutual Information audits proving zero target "
        "leakage. Enforced strict spatiotemporal group-splitting by vehicle ID (node_id) into 70% Train (16,885), 15% Val (3,546), and 15% Holdout Test (3,539).",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Benchmark of 17 Classification Architectures:</b> Evaluated across ensemble trees, gradient boosters, neural nets, linear, and non-parametric models:",
        bullet_style
    ))

    ml_perf_data = [
        [Paragraph("Model Architecture", th_style), Paragraph("Accuracy", th_style), Paragraph("F1-Weighted", th_style), Paragraph("Precision", th_style), Paragraph("Recall", th_style), Paragraph("ROC-AUC", th_style), Paragraph("Edge Latency", th_style), Paragraph("Deployment Status", th_style)],
        [Paragraph("Random Forest (Champion)", td_bold), Paragraph("90.99%", td_style), Paragraph("90.97%", td_style), Paragraph("90.98%", td_style), Paragraph("90.99%", td_style), Paragraph("90.59%", td_style), Paragraph("23.5 ms", td_style), Paragraph("C++ Native Deployed", status_done)],
        [Paragraph("Logistic Regression", td_bold), Paragraph("85.74%", td_style), Paragraph("85.62%", td_style), Paragraph("86.10%", td_style), Paragraph("85.74%", td_style), Paragraph("88.42%", td_style), Paragraph("0.12 ms", td_style), Paragraph("C++ Native Deployed", status_done)],
        [Paragraph("CatBoost Classifier", td_style), Paragraph("80.69%", td_style), Paragraph("79.79%", td_style), Paragraph("82.72%", td_style), Paragraph("80.69%", td_style), Paragraph("90.77%", td_style), Paragraph("1.45 ms", td_style), Paragraph("Benchmarked", td_style)],
        [Paragraph("LightGBM", td_style), Paragraph("72.42%", td_style), Paragraph("69.32%", td_style), Paragraph("77.52%", td_style), Paragraph("72.42%", td_style), Paragraph("89.77%", td_style), Paragraph("0.54 ms", td_style), Paragraph("C++ Predictor Built", td_style)],
        [Paragraph("XGBoost", td_style), Paragraph("71.56%", td_style), Paragraph("68.23%", td_style), Paragraph("76.62%", td_style), Paragraph("71.56%", td_style), Paragraph("89.32%", td_style), Paragraph("0.28 ms", td_style), Paragraph("Benchmarked", td_style)],
        [Paragraph("Extra Trees Classifier", td_style), Paragraph("90.82%", td_style), Paragraph("90.79%", td_style), Paragraph("90.84%", td_style), Paragraph("90.82%", td_style), Paragraph("90.31%", td_style), Paragraph("18.2 ms", td_style), Paragraph("Benchmarked", td_style)],
        [Paragraph("MLP Neural Network", td_style), Paragraph("86.12%", td_style), Paragraph("85.98%", td_style), Paragraph("86.45%", td_style), Paragraph("86.12%", td_style), Paragraph("88.90%", td_style), Paragraph("1.10 ms", td_style), Paragraph("Benchmarked", td_style)],
        [Paragraph("Stacking Ensemble", td_style), Paragraph("91.15%", td_style), Paragraph("91.12%", td_style), Paragraph("91.20%", td_style), Paragraph("91.15%", td_style), Paragraph("90.85%", td_style), Paragraph("31.4 ms", td_style), Paragraph("Benchmarked", td_style)]
    ]
    ml_perf_table = Table(ml_perf_data, colWidths=[110, 50, 60, 52, 50, 52, 58, 72], repeatRows=1)
    ml_perf_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), c_bg_subtle),
        ('GRID', (0,0), (-1,-1), 0.5, c_border),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, c_bg_alt]),
        ('TOPPADDING', (0,0), (-1,-1), 2.2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2.2),
        ('LEFTPADDING', (0,0), (-1,-1), 3.5),
        ('RIGHTPADDING', (0,0), (-1,-1), 3.5),
    ]))
    story.append(ml_perf_table)
    story.append(Spacer(1, 3))
    story.append(Paragraph(
        "• <b>Native C++ Embedded Inference:</b> Rather than executing Python bindings during OMNeT++ simulations, we compiled the trained models "
        "directly into C++ (<code>RandomForestPredictor.cc/.h</code> with 50 compiled decision trees, and <code>LogisticRegressionPredictor.cc/.h</code> "
        "with StandardScaler scaling vectors), enabling real-time zero-copy edge classification.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Full-Stack Real-Time Web Dashboard:</b> Implemented a complete Flask/Chart.js interactive dashboard (<code>dashboard/app.py</code>) "
        "featuring single-query evaluation, 17-model arena benchmarking, and live telemetry inspection.",
        bullet_style
    ))
    story.append(Spacer(1, 5))

    # Subsection 1.4: 15 VeReMi Attack Scenarios
    story.append(Paragraph("1.4 Implementation of 15+ VeReMi NextGen Attack Scenarios", h2_style))
    story.append(Paragraph(
        "Engineered autonomous attack behaviors in <code>EdgeTrustVehicleApp.cc</code> spanning all major VeReMi NextGen and VANET attack categories:",
        body_style
    ))

    attack_table_data = [
        [Paragraph("Scenario ID", th_style), Paragraph("Attack Name", th_style), Paragraph("Injected Anomaly Pattern", th_style), Paragraph("Primary Detection Mechanism", th_style)],
        [Paragraph("VeReMi 1", td_bold), Paragraph("Constant Pos Offset", td_style), Paragraph("Position coordinates shifted by fixed +28.0 m offset.", td_style), Paragraph("CAN vs BSM & Kinematic jump bound", td_style)],
        [Paragraph("VeReMi 2", td_bold), Paragraph("Random Pos Offset", td_style), Paragraph("Coordinates randomized between ±20 m and ±70 m.", td_style), Paragraph("Implied velocity violation (> 55 m/s)", td_style)],
        [Paragraph("VeReMi 3", td_bold), Paragraph("Position Mirroring", td_style), Paragraph("Reflects vehicle position across road centerline into opposite lane.", td_style), Paragraph("Spatial lane boundary & heading check", td_style)],
        [Paragraph("VeReMi 4-5", td_bold), Paragraph("Constant/Random Speed Offset", td_style), Paragraph("Claims speed offset by +12 m/s or ±6 m/s continuously.", td_style), Paragraph("Spatial displacement derivative vs reported v", td_style)],
        [Paragraph("VeReMi 6", td_bold), Paragraph("Zero Speed Report", td_style), Paragraph("Claims 0.0 m/s while vehicle is traveling at arterial speed.", td_style), Paragraph("CAN bus speed cross-validation", td_style)],
        [Paragraph("VeReMi 7-8", td_bold), Paragraph("Sudden Stop / Sudden Const Speed", td_style), Paragraph("Freezes reported position/speed while vehicle maneuvers.", td_style), Paragraph("Temporal delta-v & kinematic trajectory", td_style)],
        [Paragraph("VeReMi 9", td_bold), Paragraph("Reversed Heading", td_style), Paragraph("Heading angle flipped 180° opposite to motion direction.", td_style), Paragraph("Motion trajectory vs compass angle", td_style)],
        [Paragraph("VeReMi 10-11", td_bold), Paragraph("Feigned Braking / Accel Mult", td_style), Paragraph("False deceleration (-7 m/s²) or 3.5x acceleration multiplier.", td_style), Paragraph("Temporal consistency & calcAccel discrepancy", td_style)],
        [Paragraph("VeReMi 12", td_bold), Paragraph("DoS Broadcast Flooding", td_style), Paragraph("Floods channel with 16 Hz high-frequency packet bursts.", td_style), Paragraph("Rate anomaly filter (sliding window > 12 Hz)", td_style)],
        [Paragraph("VeReMi 13", td_bold), Paragraph("Traffic Congestion Sybil", td_style), Paragraph("Injects phantom ghost vehicle IDs to simulate fake gridlock.", td_style), Paragraph("Sybil identifier check & RSSI consistency", td_style)],
        [Paragraph("VeReMi 14-15", td_bold), Paragraph("Data Replay & Time Delay", td_style), Paragraph("Replays stale beacons or delays timestamp by +3.0 seconds.", td_style), Paragraph("Timestamp freshness & latency penalty (>500ms)", td_style)],
        [Paragraph("VANET 16", td_bold), Paragraph("Selective Blackhole Drop", td_style), Paragraph("Drops 70% of transit packets while maintaining BSM broadcasts.", td_style), Paragraph("Packet drop ratio (PDR) & comm score", td_style)]
    ]
    attack_table = Table(attack_table_data, colWidths=[55, 120, 185, 144], repeatRows=1)
    attack_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), c_bg_subtle),
        ('GRID', (0,0), (-1,-1), 0.5, c_border),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, c_bg_alt]),
        ('TOPPADDING', (0,0), (-1,-1), 2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2),
        ('LEFTPADDING', (0,0), (-1,-1), 3.5),
        ('RIGHTPADDING', (0,0), (-1,-1), 3.5),
    ]))
    story.append(attack_table)
    story.append(Spacer(1, 5))

    # Subsection 1.5: 14 Features & Trust Engine
    story.append(PageBreak())
    story.append(Paragraph("1.5 14-Feature Extraction & Dynamic Multi-Tier Trust Engine", h2_style))
    story.append(Paragraph(
        "• <b>14 Observable Feature Vector:</b> Ingests <code>[pos_x, pos_y, speed, heading, accel, pkt_sent, pkt_rcvd, pdr, latency, retx, rssi, trust_score, neighbor_trust_avg, hist_trust]</code>. "
        "Streamed simultaneously to <code>results/live_extracted_features.csv</code> and synced with <code>edgetrust-ml/data/</code>.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Mathematical Trust Model:</b> Composite evidence combines 4 factors: "
        "<i>Evidence = 0.30·Consistency + 0.30·CommScore + 0.20·NeighborValidation + 0.20·Plausibility</i>. "
        "Updated via Exponential Moving Average (EMA): <i>Trust_t = α·Trust_{t-1} + (1-α)·Evidence</i> with <b>α = 0.70</b>.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Hybrid Decision Matrix:</b> Fuses ML model classification and trust score into operational decisions: "
        "<b>ACCEPT</b> (Trust ≥ 0.70 & ML Normal), <b>WARN</b> (Trust < 0.70 or ML Malicious), <b>BLOCK</b> (Trust < 0.40 & ML Malicious).",
        bullet_style
    ))
    story.append(Spacer(1, 5))

    # Subsection 1.6: Checkpointing & I2I Communication
    story.append(Paragraph("1.6 Simulation Checkpointing & Inter-RSU (I2I) Cooperative Backbone", h2_style))
    story.append(Paragraph(
        "• <b>State Checkpointing & Warm-Start:</b> Automatically serializes full multi-tier trust states, kinematic records, attack indicators, "
        "and ML predictions into JSON (<code>checkpoint_state_rsu0.json</code>, <code>checkpoint_state_rsu1.json</code>). Simulations can be resumed "
        "instantaneously with pre-warmed trust states using the <code>LiveDemo_FromCheckpoint</code> configuration.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Periodic I2I Trust Synchronization:</b> RSUs exchange cooperative trust digests every 2.5s over an edge backbone link, "
        "aligning global reputation across Grid 1 and Grid 2, and pre-warming approaching vehicles before handover.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Urgent Threat Alerts & Preemptive Quarantine:</b> When an RSU detects a malicious node (BLOCK verdict), it immediately dispatches "
        "an urgent I2I threat alert over the edge backbone. The peer RSU pre-blacklists the rogue node before it physically crosses into the adjacent grid sector.",
        bullet_style
    ))
    story.append(Paragraph(
        "• <b>Canvas Visual Rendering:</b> Draws dynamic color-coded transmission arrows between vehicles and RSUs, along with a dashed I2I backbone line "
        "between RSU 0 and RSU 1 (cyan for periodic sync, magenta/red for urgent threat alerts).",
        bullet_style
    ))
    story.append(Spacer(1, 8))

    # ──────────────────────────────────────────────────────────────────────────
    # SECTION 2: REMAINING WORK
    # ──────────────────────────────────────────────────────────────────────────
    story.append(Paragraph("2. Remaining Work & Technical Roadmap", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=c_primary, spaceBefore=1, spaceAfter=6))

    remaining_intro = (
        "While the foundational multi-RSU simulation, OBU heuristic pre-filtering, and embedded Edge AI detection are fully functional, "
        "the following modules represent the remaining research and development tasks required to complete the project:"
    )
    story.append(Paragraph(remaining_intro, body_style))
    story.append(Spacer(1, 3))

    remaining_items = [
        ("2.1 Complete Inter-RSU Trust Synchronization Protocol",
         "Formalize and harden the I2I consensus synchronization protocol. Implement bounded convergence proofs ensuring that trust "
         "scores converge within ≤ 3.0 seconds during vehicle handover between adjacent RSU cells. Add asynchronous heartbeats, ACK-based "
         "reliable backbone transport, and loss recovery over simulated fiber/Ethernet edge links."),

        ("2.2 Duplicate, Stale, and Conflicting Trust Update Resolution",
         "In real-world decentralized VANETs, malicious nodes or asymmetric channel conditions produce conflicting trust reports. "
         "We must implement: (a) Lamport logical timestamps / sequence vectoring to discard stale or out-of-order trust packets, "
         "(b) Weighted Bayesian / Byzantine Fault Tolerant (BFT) consensus to reconcile conflicting neighbor validation scores, and "
         "(c) Sybil-resistant voting weights proportional to historical direct trust scores."),

        ("2.3 Fog-Based Regional Trust Aggregation Layer",
         "Extend the architecture into a true 3-tier hierarchical framework: Vehicle (OBU) → Edge (RSU) → Regional Fog Server. "
         "The Fog layer will aggregate trust digests across clusters of 4-10 RSUs, maintain a persistent cross-district reputation ledger, "
         "detect distributed low-and-slow multi-intersection attacks, and coordinate city-wide Certificate Revocation List (CRL) distributions."),

        ("2.4 SDN-Based Adaptive Forwarding & Dynamic QoS Rerouting",
         "Integrate Software-Defined VANET (SD-VANET) controller architecture (e.g., OpenFlow-compatible routing in OMNeT++). "
         "The controller will dynamically compute forwarding topologies based on live link trust scores, rerouting safety-critical BSM packets "
         "away from compromised or congested nodes, while dynamically provisioning bandwidth priority (QoS) for emergency advisories over non-safety traffic."),

        "PAGE_BREAK_MARKER",
        ("2.5 Multi-Density Traffic & Coordinated Multi-Vector Stress Testing",
         "Perform comprehensive simulations across three standardized traffic density regimes: Low (15 veh/km), Medium (45 veh/km), "
         "and High / Congested (90-120 veh/km). Evaluate resilience against sophisticated simultaneous multi-vector attacks (e.g., combined DoS burst "
         "flooding + Sybil phantom nodes + gradual False Data Injection drift)."),

        ("2.6 Comprehensive Empirical Evaluation & Benchmarking Suite",
         "Conduct exhaustive comparative benchmarks across the entire metric spectrum: "
         "(a) Classification metrics: Accuracy, Precision, Recall, Macro/Weighted F1-score, False Alarm Rate (FAR), Missed Detection Rate (MDR), ROC-AUC; "
         "(b) Network performance metrics: End-to-End Latency, Packet Delivery Ratio (PDR), Jitter, Control Overhead, Handover Latency; "
         "(c) Computational efficiency: CPU load, memory footprint, single-packet inference latency under load."),

        ("2.7 Scalability Benchmarking on Large-Scale Topologies",
         "Scale the simulation environment beyond the current two-RSU setup to multi-RSU arterial corridors (4 to 8 RSUs) and large-scale "
         "Manhattan urban grid topologies with 100+ concurrent vehicle trajectories to evaluate communication overhead and trust convergence scalability.")
    ]

    # Page 4 specific compact typography to ensure exact 4-page fit
    p4_h2_style = ParagraphStyle(
        'P4H2',
        parent=h2_style,
        fontSize=9.0,
        leading=11.5,
        spaceBefore=3.5,
        spaceAfter=1.5
    )
    p4_body_style = ParagraphStyle(
        'P4Body',
        parent=body_style,
        fontSize=7.6,
        leading=10.0,
        spaceAfter=2
    )
    sec3_h1_style = ParagraphStyle(
        'Sec3H1',
        parent=h1_style,
        fontSize=11,
        leading=14,
        spaceBefore=5,
        spaceAfter=2
    )

    on_page_4 = False
    for item in remaining_items:
        if item == "PAGE_BREAK_MARKER":
            story.append(PageBreak())
            on_page_4 = True
            continue
        title, desc = item
        cur_h2 = p4_h2_style if on_page_4 else h2_style
        cur_body = p4_body_style if on_page_4 else body_style
        story.append(Paragraph(f"<b>{title}</b>", cur_h2))
        story.append(Paragraph(desc, cur_body))
        story.append(Spacer(1, 1 if on_page_4 else 2))

    story.append(Spacer(1, 3))

    # ──────────────────────────────────────────────────────────────────────────
    # SECTION 3: PROJECT STATUS MATRIX & TIMELINE
    # ──────────────────────────────────────────────────────────────────────────
    story.append(Paragraph("3. Project Status Matrix & Implementation Roadmap", sec3_h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=c_primary, spaceBefore=1, spaceAfter=4))

    roadmap_th = ParagraphStyle('RoadmapTH', parent=th_style, fontSize=6.8, leading=8.5)
    roadmap_td = ParagraphStyle('RoadmapTD', parent=td_style, fontSize=6.6, leading=8.2)
    roadmap_bold = ParagraphStyle('RoadmapBold', parent=roadmap_td, fontName='Helvetica-Bold')
    roadmap_done = ParagraphStyle('RoadmapDone', parent=roadmap_td, fontName='Helvetica-Bold', textColor=c_accent_green)
    roadmap_prog = ParagraphStyle('RoadmapProg', parent=roadmap_td, fontName='Helvetica-Bold', textColor=c_accent_amber)

    roadmap_data = [
        [Paragraph("Project Component", roadmap_th), Paragraph("Key Technical Deliverables", roadmap_th), Paragraph("Status", roadmap_th), Paragraph("Target Phase", roadmap_th)],
        [
            Paragraph("Simulation Stack Integration", roadmap_bold),
            Paragraph("SUMO 1.22 + OMNeT++ 6.4 + Veins 5.2 TraCI co-simulation, IEEE 802.11p DSRC PHY/MAC.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 1", roadmap_td)
        ],
        [
            Paragraph("OBU Heuristic Pre-Filtering", roadmap_bold),
            Paragraph("4-tier local heuristic engine (Kinematics, CAN cross-check, Heading, Burst rate).", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 1", roadmap_td)
        ],
        [
            Paragraph("Machine Learning Pipeline", roadmap_bold),
            Paragraph("23,970 unified dataset, zero-leakage audit, 17 trained models, Web dashboard.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 1-2", roadmap_td)
        ],
        [
            Paragraph("Embedded C++ Predictors", roadmap_bold),
            Paragraph("Native C++ Random Forest & Logistic Regression compiled directly into RSU application.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 2", roadmap_td)
        ],
        [
            Paragraph("15+ VeReMi Attack Scenarios", roadmap_bold),
            Paragraph("Full VeReMi NextGen attack catalog + selective blackhole attack implemented in vehicles.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 2", roadmap_td)
        ],
        [
            Paragraph("14-Feature Live Extraction", roadmap_bold),
            Paragraph("Continuous feature streaming to CSV with automatic edgetrust-ml synchronization.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 2", roadmap_td)
        ],
        [
            Paragraph("Dynamic Trust Engine", roadmap_bold),
            Paragraph("Multi-tier Bayesian-EMA formulation (alpha=0.70), neighbor consensus, historical reputation.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 2", roadmap_td)
        ],
        [
            Paragraph("Simulation State Checkpointing", roadmap_bold),
            Paragraph("JSON state serialization, multi-RSU state persistence, warm-start simulation resumption.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 2", roadmap_td)
        ],
        [
            Paragraph("Two-Grid & Dual-RSU Scenario", roadmap_bold),
            Paragraph("120m corridor, 85m physical DSRC limits, primary serving vs handover observer assignment.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 2", roadmap_td)
        ],
        [
            Paragraph("Initial I2I Backbone Link", roadmap_bold),
            Paragraph("Periodic cooperative trust digest sync (2.5s) & urgent threat alert preemptive quarantine.", roadmap_td),
            Paragraph("COMPLETED", roadmap_done),
            Paragraph("Phase 2", roadmap_td)
        ],
        [
            Paragraph("Trust Sync Protocol Hardening", roadmap_bold),
            Paragraph("Formal convergence bounds, multi-hop propagation, ACK-based reliable edge transport.", roadmap_td),
            Paragraph("IN PROGRESS", roadmap_prog),
            Paragraph("Phase 3 (Active)", roadmap_td)
        ],
        [
            Paragraph("Conflict & Byzantine Resolution", roadmap_bold),
            Paragraph("Lamport timestamps, out-of-order packet mitigation, Byzantine Fault Tolerant consensus.", roadmap_td),
            Paragraph("IN PROGRESS", roadmap_prog),
            Paragraph("Phase 3", roadmap_td)
        ],
        [
            Paragraph("Fog-Based Regional Aggregation", roadmap_bold),
            Paragraph("Hierarchical OBU-RSU-Fog tier, cross-district global trust ledger, regional CRL dispatch.", roadmap_td),
            Paragraph("PLANNED", roadmap_prog),
            Paragraph("Phase 4", roadmap_td)
        ],
        [
            Paragraph("SDN Adaptive Forwarding", roadmap_bold),
            Paragraph("OpenFlow SD-VANET controller, dynamic trust-based routing, QoS priority queuing.", roadmap_td),
            Paragraph("PLANNED", roadmap_prog),
            Paragraph("Phase 4", roadmap_td)
        ],
        [
            Paragraph("Multi-Density Stress Testing", roadmap_bold),
            Paragraph("Low, Medium, and High traffic density regimes + simultaneous coordinated multi-vector attacks.", roadmap_td),
            Paragraph("PLANNED", roadmap_prog),
            Paragraph("Phase 4", roadmap_td)
        ],
        [
            Paragraph("Full Benchmark Evaluation", roadmap_bold),
            Paragraph("Accuracy, F1, FAR, MDR, PDR, End-to-end latency, Handover delay, Hardware profiling.", roadmap_td),
            Paragraph("PLANNED", roadmap_prog),
            Paragraph("Phase 5", roadmap_td)
        ],
        [
            Paragraph("Large-Scale Scalability Tests", roadmap_bold),
            Paragraph("4 to 8 RSU arterial corridor, Manhattan grid scaling, 100+ concurrent vehicles.", roadmap_td),
            Paragraph("PLANNED", roadmap_prog),
            Paragraph("Phase 5", roadmap_td)
        ]
    ]

    roadmap_table = Table(roadmap_data, colWidths=[115, 225, 84, 80], repeatRows=1)
    roadmap_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), c_bg_subtle),
        ('GRID', (0,0), (-1,-1), 0.5, c_border),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, c_bg_alt]),
        ('TOPPADDING', (0,0), (-1,-1), 1.2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 1.2),
        ('LEFTPADDING', (0,0), (-1,-1), 3.5),
        ('RIGHTPADDING', (0,0), (-1,-1), 3.5),
    ]))
    story.append(roadmap_table)
    story.append(Spacer(1, 4))

    # ── Final Sign-Off Card ───────────────────────────────────────────────────
    signoff_text = (
        "<b>Summary & Next Actions:</b> The EdgeTrust-VANET framework currently possesses an operational simulation "
        "and detection pipeline with native edge intelligence and distributed vehicle pre-filtering. The immediate focus "
        "is concluding Phase 3 (protocol hardening and Byzantine conflict resolution), followed by Phase 4 (Fog layer and SDN routing) "
        "and Phase 5 (comprehensive multi-density empirical benchmarking for final thesis publication)."
    )
    signoff_table = Table([[Paragraph(signoff_text, callout_text)]], colWidths=[504])
    signoff_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), c_card_green),
        ('BOX', (0,0), (-1,-1), 1, c_accent_green),
        ('TOPPADDING', (0,0), (-1,-1), 3.5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3.5),
        ('LEFTPADDING', (0,0), (-1,-1), 7),
        ('RIGHTPADDING', (0,0), (-1,-1), 7),
    ]))
    story.append(signoff_table)

    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"Report generated successfully: {filename}")

if __name__ == '__main__':
    out_pdf = "/home/aashiq/Desktop/FinalYearProject/EdgeTrust_Project_Progress_Report.pdf"
    build_pdf(out_pdf)
