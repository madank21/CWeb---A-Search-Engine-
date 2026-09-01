# CWeb Ranking Engine & Mathematical Formulas

## 1. Field Weighting
Each term occurrence contributes a weighted term frequency:
\[ \text{wtf}(t, d) = \sum_{\text{field}} \text{weight}(\text{field}) \times \text{raw\_count}(t, \text{field}, d) \]

Weights: Title (4.0), Heading (3.0), Keywords (3.0), Description (2.0), Body (1.0).

## 2. TF-IDF Formula
\[ \text{tf}(t, d) = 1 + \ln(\text{wtf}(t, d)) \quad \text{if } \text{wtf} > 0 \text{ else } 0 \]
\[ \text{idf}(t) = \ln\left( \frac{N + 1}{\text{df}(t) + 1} \right) + 1 \]
\[ \text{score}(d) = \sum_{t \in \text{query}} \text{tf}(t, d) \times \text{idf}(t) \]

## 3. BM25 Formula
Default parameters: \( k_1 = 1.2 \), \( b = 0.75 \).
\[ \text{idf}(t) = \ln\left( \frac{N - \text{df}(t) + 0.5}{\text{df}(t) + 0.5} + 1 \right) \]
\[ \text{score}(d) = \sum_{t \in \text{query}} \text{idf}(t) \times \frac{\text{wtf}(t, d) \cdot (k_1 + 1)}{\text{wtf}(t, d) + k_1 \cdot \left(1 - b + b \cdot \frac{|d|}{\text{avgdl}}\right)} \]

## 4. Multi-term Coordination & Tie-Breaking
Multi-term matching queries apply a coordination multiplier:
\[ \text{score}_{\text{final}} = \text{score} \times \left(1.0 + \frac{\text{matched\_terms}}{\text{total\_terms}} \times 0.5\right) \]

Ties break by:
1. Higher term coverage count.
2. More recent `modified_time`.
3. Lower `document_id`.
